/// @file macs_semaphore.cpp
/// @brief Семафоры.
/// @details Семафоры используются для организации совместного доступа нескольких задач
/// к ограниченному числу общих ресурсов. Каждый семафор хранит текущее и максимальное
/// количество доступных ресурсов, а также предоставляет задачам методы получения доступа
/// к ресурсу (Wait) и освобождения ресурса (Signal). Задачи, пытающиеся получить доступ
/// к ресурсу при отсутствии свободных ресурсов, блокируются и помещаются в очередь ожидания.
/// При этом семафор отслеживает только количество доступных ресурсов, не привязывая занятые
/// ресурсы к использующим их задачам, поэтому ожидающая задача будут разблокирована при
/// освобождении любого из ресурсов.
/// @copyright AstroSoft Ltd, 2016

#include "macs_critical_section.hpp"
#include "macs_semaphore.hpp"
 
namespace macs { 
	
Semaphore::Semaphore(size_t start_count, size_t max_count) :
	m_count(start_count <= max_count ? start_count : max_count),
	m_max_count(max_count)
{}
   
Semaphore::~Semaphore()
{}
 
Result Semaphore::Wait(uint32_t timeout_ms)
{
	if ( ! Sch().IsInitialized() || ! Sch().IsStarted() ) 
		return ResultErrorInvalidState;
	
	if ( timeout_ms == 0 ) {
		if ( ! System::IsSysCallAllowed() ) 
			return ResultErrorSysCallNotAllowed;
	} else {		
		if ( System::IsInInterrupt() ) 
			return ResultErrorInterruptNotSupported;
	}
	
	Result res = System::IsInPrivOrIrq() ? Wait_Priv(this, timeout_ms) 
	                                     : SvcExecPrivileged(this, reinterpret_cast<void*>(timeout_ms), NULL, EPM_Semaphore_Wait_Priv);	
	if ( res != ResultOk )
		return res;
	
	return Task::GetCurrent()->m_unblock_reason == Task::UnblockReasonTimeout ? ResultTimeout : ResultOk; 
}
	
Result Semaphore::Wait_Priv(Semaphore * semaphore, uint32_t timeout_ms)
{
	CriticalSection _cs_;

	Task * currentTask = Task::GetCurrent();
	if ( semaphore->TryDecrement() )	{
		currentTask->m_unblock_reason = Task::UnblockReasonNone;	// очистим это поле, по нему выше суд¤т о результате операции
		return ResultOk;
	}

	if ( timeout_ms == 0 ) 
		return ResultTimeout;

	return semaphore->BlockCurTask(timeout_ms);
}

Result Semaphore::Signal()
{ 
	if ( ! Sch().IsInitialized() || ! Sch().IsStarted() ) 
		return ResultErrorInvalidState;
	
	if ( ! System::IsSysCallAllowed() )	
		return ResultErrorSysCallNotAllowed;
	
	return System::IsInPrivOrIrq() ? Signal_Priv(this) 
	                               : SvcExecPrivileged(this, NULL, NULL, EPM_Semaphore_Signal_Priv);
}
	  
Result Semaphore::Signal_Priv(Semaphore * semaphore)
{
	CriticalSection _cs_;

	if ( semaphore->m_count == semaphore->m_max_count )	
		return ResultErrorInvalidState;

	if ( semaphore->IsHolding() ) 
		return semaphore->UnblockTask();

	++ semaphore->m_count;

	return ResultOk;
}

}	// namespace macs
