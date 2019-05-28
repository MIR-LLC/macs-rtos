/// @file macs_mutex.cpp
/// @brief Мьютексы.
/// @details Мьютекс (MUTualEXclusive) – это объект синхронизации, у которого в каждый
/// момент времени может быть только один владелец. Все остальные задачи, пытающиеся
/// захватить мьютекс, будут заблокированы до освобождения мьютекса. Если при освобождении
/// мьютекса происходит разблокировка ожидающей его задачи, то владение мьютексом переходит
/// к разблокированной задаче. 
/// @copyright AstroSoft Ltd, 2016

#include <stdint.h> 

#include "macs_mutex.hpp"
#include "macs_critical_section.hpp"
#include "macs_application.hpp"

namespace macs {
	
Mutex::~Mutex()
{
	if ( m_owner ) { 
		MACS_ALARM(AR_OWNED_MUTEX_DESTR);
		m_owner->RemoveOwnedSync(this);
	}
	if ( IsHolding() ) {
		MACS_ALARM(AR_BLOCKING_MUTEX_DESTR);
		DropLinks();
	}
}

Result Mutex::Lock(uint32_t timeout_ms)
{
	if ( System::IsInInterrupt() ) 
		return ResultErrorInterruptNotSupported;

	Result res = System::IsInPrivOrIrq() ? Lock_Priv(this, timeout_ms) 
	                                     : SvcExecPrivileged(this, reinterpret_cast<void*>(timeout_ms), NULL, EPM_Mutex_Lock_Priv);
	if ( res != ResultOk )
		return res;
	
	return Task::GetCurrent()->m_unblock_reason == Task::UnblockReasonTimeout ? ResultTimeout : ResultOk; 
}
	 
Result Mutex::Lock_Priv(Mutex * mutex, uint32_t timeout_ms)
{
	CriticalSection _cs_;

	Task * cur_task = Task::GetCurrent();
	if ( mutex->m_owner == cur_task ) {	// мьютекс уже был заблокирован нами
		if ( ! cur_task ) 	// попытка блокировки из кода вне контекста задачи
			return ResultErrorInvalidState;

		if ( mutex->m_owner && ! mutex->m_recursive ) {	// повторная блокировка нерекурсивного мьютекса
			MACS_ALARM(AR_NESTED_MUTEX_LOCK);
			return ResultErrorInvalidState;
		}
		  
		_ASSERT(mutex->m_lock_cnt > 0);
				
		if ( mutex->m_lock_cnt == BYTE_MAX ) {
			MACS_ALARM(AR_COUNTER_OVERFLOW);
			return ResultErrorInvalidState;
		}

		++ mutex->m_lock_cnt;
		return ResultOk;
	}
	// либо мьютекс не занят, либо занят другой задачей
	if ( mutex->m_owner == nullptr ) {	// мьютекс свободен
		mutex->m_owner = cur_task;
#if MACS_MUTEX_PRIORITY_INVERSION
		mutex->m_owner_original_priority = cur_task->m_owned_obj_list ? 
		                                  cur_task->m_owned_obj_list->m_owner_original_priority :
		                                  cur_task->GetPriority();
#endif
		cur_task->AddOwnedSync(mutex);
			
		_ASSERT(mutex->m_lock_cnt == 0);
		mutex->m_lock_cnt = 1;
		
		cur_task->m_unblock_reason = Task::UnblockReasonNone;	// очистим это поле, по нему выше судят о результате операции

		return ResultOk;
	}
	// мьютекс занят другой задачей
	if ( timeout_ms == 0 ) 
		return ResultTimeout;

	return mutex->BlockCurTask(timeout_ms);
}
 
Result Mutex::Unlock()
{
	if ( System::IsInInterrupt() ) 
		return ResultErrorInterruptNotSupported;

	return System::IsInPrivOrIrq() ? Unlock_Priv(this) 
	                               : SvcExecPrivileged(this, NULL, NULL, EPM_Mutex_Unlock_Priv);
}
	
Result Mutex::Unlock_Priv(Mutex * mutex)
{
	CriticalSection _cs_;

	Task * cur_task = Sch().GetCurrentTask();
	if ( ! cur_task || mutex->m_owner != cur_task ) 
		return ResultErrorInvalidState;
	 
	_ASSERT(mutex->m_lock_cnt > 0);
	
	if ( --mutex->m_lock_cnt > 0 )
		return ResultOk;

#if MACS_MUTEX_PRIORITY_INVERSION
	Task::Priority inh_prior = mutex->RemoveFromOwner();
	if ( mutex->m_owner->GetPriority() != inh_prior )
		_SetTaskPriority_Priv(& Sch(), mutex->m_owner, inh_prior);
#else
	mutex->RemoveFromOwner();
#endif
	if ( mutex->IsHolding() )
		return mutex->UnblockTask();
	mutex->m_owner = nullptr;
	return ResultOk;
}

#if	MACS_MUTEX_PRIORITY_INVERSION

extern Result IntSetTaskPriority_Priv(Scheduler * scheduler, Task * task, Task::Priority priority, bool internal_usage);

void Mutex::UpdateOwnerPriority()
{
	Task::Priority maxPriority = m_owner_original_priority;
	if ( IsHolding() ) {
		Task::Priority maxBlockedPriority = m_blocked_task_list->GetPriority();
		if ( maxBlockedPriority > maxPriority )
			maxPriority = maxBlockedPriority;
	}

	if ( m_owner->GetPriority() != maxPriority ) 
		IntSetTaskPriority_Priv(& Sch(), m_owner, maxPriority, true);
}
#endif


Result Mutex::UnlockInternal()
{
#if MACS_MUTEX_PRIORITY_INVERSION
	Task::Priority inh_prior = RemoveFromOwner();
	if ( m_owner->GetPriority() != inh_prior ) 
		IntSetTaskPriority_Priv(& Sch(), m_owner, inh_prior, true);
#else
	RemoveFromOwner();
#endif
	if ( IsHolding() ) 
		return UnblockTask(); 

	m_owner = nullptr;

	return ResultOk;
}


Result Mutex::BlockCurTask(uint32_t timeout_ms)
{
	Result res = SyncObject::BlockCurTask(timeout_ms);
#if	MACS_MUTEX_PRIORITY_INVERSION
	UpdateOwnerPriority();
#endif
	return res;
}
	 
Result Mutex::UnblockTask() 
{
	_ASSERT(IsHolding());
	m_owner = TaskSyncList::Fetch(m_blocked_task_list);
	_ASSERT(m_lock_cnt == 0);
	m_lock_cnt = 1;
	m_owner->AddOwnedSync(this);
#if MACS_MUTEX_PRIORITY_INVERSION
	m_owner_original_priority = m_owner->GetPriority();
#endif
	return Sch().UnblockTask(m_owner);
}

void Mutex::OnUnblockTask(Task * task, Task::UnblockReason reason) 
{
	if ( reason == Task::UnblockReasonTimeout ) {
		TaskSyncList::Del(m_blocked_task_list, task);
#if	MACS_MUTEX_PRIORITY_INVERSION
		UpdateOwnerPriority();
#endif
	}
}
 
void Mutex::OnDeleteTask(Task * task)
{
	if ( ! m_owner )
		return;

	if ( task != m_owner ) {
		SyncObject::OnDeleteTask(task);
#if	MACS_MUTEX_PRIORITY_INVERSION
		UpdateOwnerPriority();
#endif
	} else {
		UnlockInternal();
		m_lock_cnt = 0;
	}
}

}	// namespace macs
