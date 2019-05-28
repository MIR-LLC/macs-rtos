/// @file macs_task.cpp
/// @brief Задачи.
/// @details Предоставляет базовые классы для реализации задач. Любая задача в приложении
/// должна быть унаследована от соответствующего базового класса. В унаследованном классе
/// следует переопределить виртуальный метод, содержащий код задачи.
/// @copyright AstroSoft Ltd, 2016

#include <string.h>
#include <stddef.h>
 
#include "macs_memory_manager.hpp"
#include "macs_task.hpp"
#include "macs_critical_section.hpp"
#include "macs_scheduler.hpp"
#include "macs_stack_frame.hpp"
   
namespace macs {
	
void Task::Init(const char * name, size_t stack_len, uint32_t * stack_mem)
{
	_ASSERT(!! stack_len == !! stack_mem);	
	m_stack.Build(stack_len, stack_mem);
	 
	m_priority = PriorityNormal;
	m_state = StateInactive;
#if MACS_PROFILING_ENABLED		// все задачи будут привилегированными по умолчанию
	m_mode = ModePrivileged;
#else		
	m_mode = ModeUnprivileged;
#endif		
	
#if MACS_USE_CLOCK
	m_run_duration.Zero();
#endif
	 
	m_dream_ticks = 0;
	m_next_sched_task = nullptr;
	m_next_sync_task = nullptr;
	m_unblock_func = nullptr;
	m_owned_obj_list = nullptr;
	m_unblock_reason = UnblockReasonNone;
	
	if ( name )	{
#if MACS_TASK_NAME_LENGTH > 0	 
		strncpy(m_name_arr, name, MACS_TASK_NAME_LENGTH);
#elif MACS_TASK_NAME_LENGTH == -1	  	
		m_name_ptr = new char[strlen(name) + 1];
		strcpy(const_cast<char *>(m_name_ptr), name);
#endif					
	} else {
#if MACS_TASK_NAME_LENGTH == -1	  	
		m_name_ptr = nullptr;
#endif					
	}		
#if MACS_TASK_NAME_LENGTH > 0	 
	m_name_arr[MACS_TASK_NAME_LENGTH] = '\0';
#endif					
}
	  
Task::~Task()
{
	if ( m_state != StateInactive ) 
		Sch().DeleteTask(this, false);
	
#if MACS_TASK_NAME_LENGTH == -1	  	
	if ( m_name_ptr )
		delete[] m_name_ptr;
#endif					
}

void Task::InitializeStack(size_t stack_size, void (* onTaskExit)(void))
{
	m_stack.Prepare(stack_size, this, reinterpret_cast<void (*)()>(GetExecuteAddress()), onTaskExit);
}

Result Task::Add(Task * task, Task::Priority priority, Task::Mode mode, size_t stack_size)
{
	return Sch().AddTask(task, priority, mode, stack_size); // Обработка профайлера в Scheduler
}

Result Task::Remove()
{
	return Sch().DeleteTask(this, false);
}

Result Task::Delete()
{
	return Sch().DeleteTask(this, true);
}

Result Task::Delay(uint32_t timeout_ms)
{
	return Sch().BlockCurrentTask(timeout_ms);
}

void Task::CpuDelay(uint32_t timeout_ms)
{
	uint32_t timeout_ticks = MsToTicks(timeout_ms);
	uint32_t ticks = Sch().GetTickCount();
	while (Sch().GetTickCount() - ticks < timeout_ticks)
		;
}

Result Task::SetPriority(Priority value)
{
	return Sch().SetTaskPriority(this, value);
}

Task * Task::GetCurrent()
{
	return Sch().GetCurrentTask();
}

void Task::Yield()
{
	Sch().Yield();
}

void Task::SetBlockSync(SyncObject * sync_obj)
{
	_ASSERT(sync_obj);
	_ASSERT(! m_unblock_func);
	m_unblock_func = sync_obj;
}

void Task::AddOwnedSync(SyncOwnedObject * sync_obj)
{
	_ASSERT(sync_obj);
	_ASSERT(! * OwnedSyncObjList::Find(m_owned_obj_list, sync_obj));
	OwnedSyncObjList::Add(m_owned_obj_list, sync_obj);
}

void Task::DropBlockSync(SyncObject * sync_obj)
{
	_ASSERT(sync_obj);
	_ASSERT(m_unblock_func == sync_obj);
	m_unblock_func = nullptr;
}

void Task::RemoveOwnedSync(SyncOwnedObject * sync_obj)
{
	_ASSERT(sync_obj);
	_ASSERT(* OwnedSyncObjList::Find(m_owned_obj_list, sync_obj));

	OwnedSyncObjList::Del(m_owned_obj_list, sync_obj);
}

void Task::DetachFromSync()
{
	if ( m_unblock_func ) {
		m_unblock_func->OnDeleteTask(this);
		m_unblock_func = nullptr;
	}
	while ( m_owned_obj_list ) 
		m_owned_obj_list->OnDeleteTask(this);
}

void PrintPriority(String & str, Task::Priority prior, bool brief)
{
	switch ( prior ) {
	case Task::PriorityIdle :         str = brief ? "ID" : "Idle";        break;
	case Task::PriorityLow :          str = brief ? "LO" : "Low";         break;
	case Task::PriorityBelowNormal :  str = brief ? "BN" : "BelowNormal"; break;
	case Task::PriorityNormal :       str = brief ? "NM" : "Normal";      break;
	case Task::PriorityAboveNormal :  str = brief ? "AN" : "AboveNormal"; break;
	case Task::PriorityHigh :         str = brief ? "HI" : "High";        break;
	case Task::PriorityRealtime :     str = brief ? "RT" : "Realtime";    break;
	case Task::PriorityInvalid :      str = brief ? "IN" : "Invalid";     break;
	default :	
		{
			String prior_str = (CSPTR) PrnFmt("%d", prior);
			if ( ! brief ) 
				str.Clear() << "Priority(" << prior_str << ")";
			else 
				str = prior_str;
		}
		break;
	}
}
 
Result SyncObject::BlockCurTask(uint32_t timeout_ms)
{
	Task * cur_task = Sch().GetCurrentTask();
	TaskSyncList::Add(m_blocked_task_list, cur_task);
	cur_task->SetBlockSync(this);
	return BlockCurrentTask_Priv(& Sch(), timeout_ms, this);
}
	 
Result SyncObject::UnblockTask()
{
	_ASSERT(IsHolding());
	Task * task = TaskSyncList::Fetch(m_blocked_task_list);
	task->DropBlockSync(this);
	return Sch().UnblockTask(task);
}
	
void SyncObject::DropLinks()
{
	while ( m_blocked_task_list ) {
		Task * task = TaskSyncList::Fetch(m_blocked_task_list);
		task->DropBlockSync(this);
	}
}
 
void SyncObject::OnUnblockTask(Task * task, Task::UnblockReason reason)
{
	if ( reason == Task::UnblockReasonTimeout ) {
		TaskSyncList::Del(m_blocked_task_list, task);
		task->DropBlockSync(this);
	}
}
	
void SyncObject::OnDeleteTask(Task * task)
{
	TaskSyncList::Del(m_blocked_task_list, task);
}

TaskIrq::TaskIrq(const char* name) : 
	Task(name) 
{ 
	m_irq_up = false; 
	m_irq_num = -1; 
	m_next_irq_task = nullptr; 
}		
	
Result TaskIrq::Add(TaskIrq * task, int irq_num, Task::Priority priority, Task::Mode mode, size_t stack_size) 
{
	return Sch().AddTask(task, irq_num, priority, mode, stack_size);
}
	
void TaskIrq::Execute()
{
	for(;;) { 
		IrqHandler();
		Delay(INFINITE_TIMEOUT);
	}
} 

// Для вызова из С кода.
extern "C" {
	tick_t MacsGetTickCount() { return Sch().GetTickCount(); }
	void MacsDelay(ulong ticks)  	{ Task::Delay(ticks); }
	void MacsCpuDelay(ulong ticks)	{ Task::CpuDelay(ticks); }
}

} // namespace macs

