/// @file macs_scheduler.cpp
/// @brief Планировщик.
/// @details Содержит описание планировщика и связанного с ним функционала. 
/// @copyright AstroSoft Ltd, 2016
  
#include <stdint.h>
#include <limits.h>
#include <stddef.h>
 
#include "macs_common.hpp"

#include "macs_system.hpp"

#include "macs_memory_manager.hpp"
#include "macs_scheduler.hpp"
#include "macs_critical_section.hpp"
#include "macs_stack_frame.hpp"
#include "macs_mutex.hpp"
#include "macs_semaphore.hpp"
#include "macs_event.hpp"
#include "macs_list.hpp"
#include "macs_profiler.hpp"
#if MACS_USE_LOG
	#include "macs_log.hpp"
#endif	

namespace macs {
	  
// вектор для вызова методов через SVC в привилегированном режиме (см. SVC_Handler.s)
extern "C" void * svcMethods[] =
{ 
	reinterpret_cast<void *>(EPM_Count),
	reinterpret_cast<void *>(& Read_Cpu_Tick_Priv),
	reinterpret_cast<void *>(& BlockCurrentTask_Priv),
	reinterpret_cast<void *>(& AddTask_Priv),
	reinterpret_cast<void *>(& AddTaskIrq_Priv),
	reinterpret_cast<void *>(& Yield_Priv),
	reinterpret_cast<void *>(& DeleteTask_Priv),
	reinterpret_cast<void *>(& UnblockTask_Priv),
	reinterpret_cast<void *>(& SetTaskPriority_Priv),
	reinterpret_cast<void *>(& Event::Raise_Priv),
	reinterpret_cast<void *>(& Event::Wait_Priv),
	reinterpret_cast<void *>(& Mutex::Lock_Priv),
	reinterpret_cast<void *>(& Mutex::Unlock_Priv),
	reinterpret_cast<void *>(& Semaphore::Wait_Priv), 
	reinterpret_cast<void *>(& Semaphore::Signal_Priv)
#if MACS_SHARED_MEM_SPI			
	,
	reinterpret_cast<void *>(& Spi_Initialize_Priv),
	reinterpret_cast<void *>(& Spi_PowerControl)
#endif			
}; 
	
extern "C" void SvcInitScheduler();

#if MACS_DEBUG	
unsigned long IdleTaskCnt; // Глобальная переменная для отладки
#endif	
class IdleTask : public Task
{
public:
	IdleTask() : Task("IDLE") { }

private:	
	virtual void Execute() { 
		for(;;) {
#if MACS_SLEEP_ON_IDLE
			System::EnterSleepMode();
#endif			
#if MACS_DEBUG	
			++ IdleTaskCnt;
#endif			
		}
	}
};

Scheduler::Scheduler() :
	m_cur_task(nullptr),
	m_tick_count(0),
	m_initialized(false),
	m_started(false),
	m_pause_flg(false),
	m_pause_cnt(0),
	m_pending_swc(false),
	m_use_preemption(true)
{}
Scheduler Scheduler::m_instance;

Scheduler::~Scheduler()
{
	// планировщик не может быть удалён до завершения программы,
	// поэтому нет смысла очищать ресурсы
}

Result Scheduler::Initialize()
{
	if ( System::IsInInterrupt() )
		return ResultErrorInterruptNotSupported;

	if ( m_initialized )
		return ResultErrorInvalidState;

#if MACS_PROFILING_ENABLED 
	TuneProfiler(); 
#endif
		
#if MACS_USE_MPU
	MPU_Init();		
#endif		
	 
	m_tick_count = 0;

	if ( ! System::InitScheduler() )
		return ResultErrorInvalidState;

	// может возникнуть ситуация, когда все задачи заблокированы 
	// (ждут событий синхронизации или функцией Delay),
	// но процессор всегда должен выполнять какой-либо код, 
	// поэтому в данном случае управление будет передано задаче бездействие;
	// у неё самый низкий приоритет, поэтому, если есть какая-либо другая задача 
	// более высокого приоритета, то IdleTask не получит процессорного времени
	AddTask(new IdleTask(), Task::PriorityIdle, Task::ModePrivileged);
	
	m_initialized = true;
	return ResultOk;
}

Result Scheduler::Start(bool use_preemption)
{
	if ( System::IsInInterrupt() )
		return ResultErrorInterruptNotSupported;

	if ( ! m_initialized || m_started )
		return ResultErrorInvalidState;

	// метод должен быть вызван в привилегированном режиме с использованием стека MSP,
	// если это не так, то выходим (возможно потом стоит предусмотреть вариант, когда
	// привелигированный режим и стек PSP)
	if ( ! System::IsInPrivMode() || ! System::IsInMspMode() )
		return ResultErrorInvalidState;
	 
	m_use_preemption = use_preemption;

	// будет выбрана первая задача
	SelectNextTask();

#if MACS_MPU_PROTECT_STACK		
	m_cur_task->m_stack.SetMpuMine();	
#endif
		
	m_started = true;

#if MACS_USE_CLOCK
	m_cur_task->m_switch_cpu_tick = System::AskCurCpuTick();
#endif
	  
	System::FirstSwitchToTask(m_cur_task->m_stack.m_top, m_cur_task->m_mode == Task::ModePrivileged);
	 
	// сюда не должны попасть
	return ResultOk;
}

Result Scheduler::Pause(bool set_on)
{
	if ( ! m_started )
		return ResultErrorInvalidState;
	
	if ( ! set_on ) {
		// Операции со счетчиком безопасны, т.к. сюда можно попасть только во время паузы в текущей задаче
		if ( m_pause_cnt == 0 ) {
			MACS_ALARM(AR_SCHED_NOT_ON_PAUSE);
			return ResultErrorInvalidState;
		} 
		if ( -- m_pause_cnt == 0 ) {
			if ( m_pending_swc ) {
#if MACS_USE_CLOCK
				Clock::OnTick(m_tick_count);
#endif					
				Yield();
			}
		} 
	} else {
		// Анализ показывает, что достаточно одного счётчика, без флага.
		// Но пока это строго не доказано и нет соответствующих тестов, пусть остаётся так.
		m_pause_flg = true;		// Теперь планировщик уже на паузе за счёт флага
		++ m_pause_cnt;				// Увеличение счётчика безопасно
		if ( m_pause_cnt == 0 )
			MACS_ALARM(AR_COUNTER_OVERFLOW);
		m_pause_flg = false;	// Планировщик остается на паузе из-за счётчика
	}
	
	return ResultOk;
}
 
void Scheduler::ForceContextSwitch()
{
	// если вызов данной функции происходит внутри критической секции,
	// то RequestContextSwitch лишь установит флаг того, что необходимо
	// вызвать соответствующее прерывание, но фактический его вызов
	// будет выполнен только после завершения критической секции;
	// чтобы решить данную проблему, мы создаём "разрыв" в критической секции
	// CriticalSectionGap, в его конструкторе прерывания будут разрешены
	// и процессор сразу вызовет прерывание для переключения контекста,
	// после того как выполнение кода снова вернётся сюда и данная функция завершится,
	// объект CriticalSectionGap уничтожится и критическая секция будет восстановлена

	System::SwitchContext();
	//CriticalSectionGap csg;
}

void Yield_Priv(Scheduler * scheduler)
{
	CriticalSection _cs_;

	if ( scheduler->IsContextSwitchRequired() )	
		scheduler->TryContextSwitch();
}

static void OnTaskExit()
{
	// при завершении задача удалит сама себя

	Scheduler & scheduler = Sch();
	scheduler.RemoveTask(scheduler.GetCurrentTask());

	// сюда не должны попасть
}

Result Scheduler::AddTask(Task * task, Task::Priority priority, Task::Mode mode, size_t stack_size)
{
	if ( System::IsInInterrupt() && ! System::IsInSysCall() ) 
		return ResultErrorInterruptNotSupported;

	if ( ! task || ! IsPriorityValid(priority) ) 
		return ResultErrorInvalidArgs;

	if ( task->m_state != Task::StateInactive ) 
		return ResultErrorInvalidState;

	task->InitializeStack(stack_size, OnTaskExit);
	task->m_priority = priority;
	task->m_state = Task::StateReady;
#if MACS_PROFILING_ENABLED 
	task->m_mode = Task::ModePrivileged;
#else		
	task->m_mode = mode;
#endif		

	return System::IsInPrivOrIrq() ? AddTask_Priv(this, task) 
	                               : SvcExecPrivileged(this, task, NULL, EPM_AddTask_Priv);
}
 
Result AddTask_Priv(Scheduler * scheduler, Task * task)
{
	CriticalSection _cs_;

	scheduler->m_work_tasks.Insert(task);

#if MACS_USE_LOG
	if ( LogOsEvent::s_os_events_reg.Check(LogOsEvent::TASK_ADDED) ) 
		 (new LogOsEvent(LogOsEvent::TASK_ADDED, task->GetName()))->Reg();	
#endif
	
	if ( scheduler->m_use_preemption )	
		scheduler->Yield();

	return ResultOk;
}

Result Scheduler::AddTask(TaskIrq * task, int irq_num, Task::Priority priority, Task::Mode mode, size_t stack_size)
{
	if ( System::IsInInterrupt() && ! System::IsInSysCall() )
		return ResultErrorInterruptNotSupported;

	if ( ! task || ! IsPriorityValid(priority) )
		return ResultErrorInvalidArgs;

	if ( task->m_state != Task::StateInactive )
		return ResultErrorInvalidState;
	
	task->InitializeStack(stack_size, OnTaskExit);
	_ASSERT(task->m_irq_num == -1);		
	task->m_irq_num = irq_num;
	task->m_priority = priority;
	task->m_state = Task::StateBlocked;
#if MACS_PROFILING_ENABLED 
		task->m_mode = Task::ModePrivileged;
#else		
		task->m_mode = mode;
#endif		
		
	return System::IsInPrivOrIrq() ? AddTaskIrq_Priv(this, task) 
	                               : SvcExecPrivileged(this, task, NULL, EPM_AddTaskIrq_Priv);
}
	
extern "C" void MacsIrqHandler() 
{
	CriticalSection _cs_;	
	int inum = System::CurIrqNum();
	Sch().ProceedIrq(inum);
}
	 
Result AddTaskIrq_Priv(Scheduler * scheduler, TaskIrq * task)
{
	CriticalSection _cs_;
	
	scheduler->m_irq_tasks.Add(task);

	return ResultOk;
}

Result DeleteTask_Priv(Scheduler * scheduler, Task * task, bool del_mem)
{
	CriticalSection _cs_;

	if ( task->m_state == Task::StateInactive ) 
		return ResultErrorInvalidState;

	const bool is_suicide = (task == scheduler->m_cur_task);

	if ( ! is_suicide ) {
		scheduler->m_sleep_tasks.Remove(task);	
		if ( task->IsRunnable() ) 
			scheduler->m_work_tasks.Remove(task);
	}
		
	task->DetachFromSync();
		
	scheduler->m_irq_tasks.Del((TaskIrq *) task);

#if MACS_MPU_PROTECT_STACK		
	if ( is_suicide )  
		MPU_RemoveMine(MPU_PROC_STACK_MINE);
#endif
		
	task->m_state = Task::StateInactive;
	
#if MACS_USE_LOG
	if ( LogOsEvent::s_os_events_reg.Check(LogOsEvent::TASK_REMOVED) ) 
		 (new LogOsEvent(LogOsEvent::TASK_REMOVED, task->GetName()))->Reg();	
#endif

	if ( del_mem ) 
		delete task;		
	
	if ( is_suicide ) {
		scheduler->m_cur_task = nullptr;		// Иначе при переключении контекста он используется!
		//scheduler->TryContextSwitch();  			дефект 41760
		System::InternalSwitchContext(); 	// если удалили себя, то планировщик уже запущен - сразу делаем переключение контектста
	}

	return ResultOk;
}
 
Result Scheduler::DeleteTask(Task * task, bool del_mem)
{
	if ( System::IsInInterrupt() ) 
		return ResultErrorInterruptNotSupported;

	if ( ! task )	
		return ResultErrorInvalidArgs;

	if (m_started) // дефект 42617
		// Нельзя удалять задачу на ее собственном стеке, так как он удаляется вместе с ней
		return SvcExecPrivileged(this, task, reinterpret_cast<void*>(del_mem), EPM_DeleteTask_Priv);
	return DeleteTask_Priv(this, task, del_mem);	// дефект 42617 до старта планировщика стек уж точно не этой задачи
}
  
Result Scheduler::BlockCurrentTask(uint32_t timeout_ms, Task::UnblockFunctor * unblock_functor)
{
	Result res = System::IsInPrivOrIrq() ? BlockCurrentTask_Priv(this, timeout_ms, unblock_functor) 
	                                     : SvcExecPrivileged(this, reinterpret_cast<void*>(timeout_ms), unblock_functor, EPM_BlockCurrentTask_Priv);
	if ( res != ResultOk )
		return res;
	
	return m_cur_task->m_unblock_reason == Task::UnblockReasonTimeout ? ResultTimeout : ResultOk;
}

Result BlockCurrentTask_Priv(Scheduler * scheduler, uint32_t timeout_ms, Task::UnblockFunctor * unblock_functor)
{
	if ( ! scheduler->m_started ) 
		return ResultErrorInvalidState;

	if ( System::IsInInterrupt() && ! System::IsInSysCall() ) 
		return ResultErrorInterruptNotSupported;

	CriticalSection _cs_;

	if ( ! scheduler->m_cur_task->IsRunnable() ) 
		return ResultErrorInvalidState;

	// хотя эту проверку можно выполнить до входа в критическую секцию,
	// но её нужно делать только если состояние задачи корректно, т.к.
	// если оно некорректно то результат должен быть ResultErrorInvalidState,
	// а не ResultTimeout
	if ( timeout_ms == 0 ) {
		if ( unblock_functor )
			unblock_functor->OnUnblockTask(scheduler->m_cur_task, Task::UnblockReasonTimeout);
		
		return ResultTimeout;
	}

	scheduler->m_cur_task->m_state = Task::StateBlocked;
	scheduler->m_cur_task->m_unblock_reason = Task::UnblockReasonNone;
	scheduler->m_cur_task->m_unblock_func = unblock_functor;

	scheduler->m_cur_task->m_dream_ticks = (timeout_ms != INFINITE_TIMEOUT ? MsToTicks(timeout_ms) : ULONG_MAX);
	scheduler->m_sleep_tasks.Insert(scheduler->m_cur_task);

	scheduler->TryContextSwitch();

	//return scheduler->_currentTask->_unblockReason == Task::UnblockReasonTimeout ? ResultTimeout : ResultOk;
	return ResultOk;
}

Result UnblockTask_Priv(Scheduler * scheduler, Task * task)
{
	CriticalSection _cs_;

	// если задача была заблокирована на определённое время, но была разблокирована
	// до того как время наступило, то она ещё может находиться в списке отложенных задач
	scheduler->m_sleep_tasks.Remove(task);
		
	if ( ! scheduler->UnblockTaskInternal(task, Task::UnblockReasonRequest) ) 
		return ResultErrorInvalidState;

	if ( ! scheduler->m_use_preemption ) 
		return ResultOk;

	if ( scheduler->m_cur_task->m_priority < task->m_priority ) 
		scheduler->TryContextSwitch();

	return ResultOk;
}

Result Scheduler::UnblockTask(Task * task)
{
	if ( ! m_started ) 
		return ResultErrorInvalidState;

	if ( ! System::IsSysCallAllowed() ) 
		return ResultErrorSysCallNotAllowed;

	if ( ! task ) 
		return ResultErrorInvalidArgs;
	
	return System::IsInPrivOrIrq() ? UnblockTask_Priv(this, task) 
	                               : SvcExecPrivileged(this, task, NULL, EPM_UnblockTask_Priv);
}

bool Scheduler::UnblockTaskInternal(Task * task, Task::UnblockReason reason)
{
	if ( task->m_state != Task::StateBlocked )
		return false;

	task->m_unblock_reason = reason;
	task->m_state = Task::StateReady;
	if ( task != m_cur_task )
		m_work_tasks.Insert(task);

	if ( task->m_unblock_func )	{
		task->m_unblock_func->OnUnblockTask(task, reason);
		task->m_unblock_func = nullptr;
	}

	return true;
}


void _SetTaskPriority_Priv(Scheduler * scheduler, Task * task, Task::Priority priority)
{
	task->m_priority = priority;
	if ( scheduler->m_use_preemption )
		scheduler->Yield();
}


Result SetTaskPriority_Priv(Scheduler * scheduler, Task * task, Task::Priority priority)
#if MACS_MUTEX_PRIORITY_INVERSION
{
	return IntSetTaskPriority_Priv(scheduler, task, priority, false);
}
Result IntSetTaskPriority_Priv(Scheduler * scheduler, Task * task, Task::Priority priority, bool internal_usage)
#endif
{
	CriticalSection _cs_;

	if ( task->m_state == Task::StateInactive )
		return ResultErrorInvalidState;

	if ( task->m_priority == priority )
		return ResultOk;

	task->m_priority = priority;

	if ( task->m_state == Task::StateReady ) {
		scheduler->m_work_tasks.Remove(task);	// Эта последовательность поместит задачу на нужное место
		scheduler->m_work_tasks.Insert(task);
	}

#if MACS_MUTEX_PRIORITY_INVERSION
	if (!internal_usage) {
		SyncOwnedObject * pobj = task->m_owned_obj_list;
		while ( pobj ) {
			pobj->m_owner_original_priority = priority;
			pobj = OwnedSyncObjList::Next(pobj);
		}
	}
#endif

	if ( scheduler->m_use_preemption )
		scheduler->Yield();

	return ResultOk;
}

Result Scheduler::SetTaskPriority(Task * task, Task::Priority priority)
{
	if ( ! m_started ) 
		return ResultErrorInvalidState;

	if ( System::IsInInterrupt() )
		return ResultErrorInterruptNotSupported;

	if ( ! task || ! IsPriorityValid(priority) )
		return ResultErrorInvalidArgs;

	return System::IsInPrivOrIrq() ? SetTaskPriority_Priv(this, task, priority) 
	                               : SvcExecPrivileged(this, task, reinterpret_cast<void*>(priority), EPM_SetTaskPriority_Priv);
}

bool Scheduler::IsPriorityValid(Task::Priority priority)
{
	// т.к. значения приоритетов начинаются с 0, то для Task::Priority компилятором
	// был выбран тип unsigned, поэтом проверка priority >= Task::PriorityIdle не требуется
	return priority <= Task::PriorityMax;
}
 
void Scheduler::TuneProfiler()
{
#if MACS_PROFILING_ENABLED 
	ProfEye::Tune();
#endif	
}
	
bool Scheduler::SysTickHandler()
{
	CriticalSection _cs_;
	++ m_tick_count;

#if MACS_USE_CLOCK
	if ( ! m_pause_cnt )	
		Clock::OnTick(m_tick_count);
#endif	

	if ( ! m_started )
		return false;

	m_sleep_tasks.Tick();
	for(;;) {
		Task * awake_task = m_sleep_tasks.Fetch();
		if ( ! awake_task )
			break;
		if ( ! UnblockTaskInternal(awake_task, Task::UnblockReasonTimeout) ) {
				;
		}
	}

#if ! MACS_IRQ_FAST_SWITCH		
	if ( m_irq_tasks.NeedIrqActivate() )
		m_irq_tasks.ActivateTasks();
#endif

	if ( m_pause_flg || m_pause_cnt != 0 )	{
		m_pending_swc = true;
		return false;	
	}

	if ( ! m_use_preemption )
		return false;

	return IsContextSwitchRequired();
}

bool Scheduler::IsContextSwitchRequired()
{
	// если есть всего одна высокоприоритетная задача, выполняющаяся в данный момент,
	// то при переключении контекста планировщик переключится опять на неё же,
	// чтобы исключить это бессмысленное действие выполняются следующие проверки
		
	if ( m_pending_swc ) 
		return true;

	if ( ! m_cur_task || m_cur_task->m_state != Task::StateRunning )
		return true;

	const Task * cand_task = m_work_tasks.FirstTask();
	if ( cand_task && m_cur_task->GetPriority() <= cand_task->GetPriority() )			
		return true;		
		
	return false;
}

// Только для вызова из критической секции!
void Scheduler::SelectNextTask()
{
	if ( m_cur_task ) {
		if ( m_cur_task->m_state == Task::StateRunning ) 
			m_cur_task->m_state = Task::StateReady;
		
		if ( m_cur_task->m_state == Task::StateReady ) 
			m_work_tasks.Insert(m_cur_task);		
	}
	
	m_cur_task = m_work_tasks.Fetch();
	m_cur_task->m_state = Task::StateRunning;
}

// new_sp - текущее значение SP задачи, которая переключается из состояния выполнения; оно будет сохранено в объекте задачи
StackPtr Scheduler::SwitchContext(StackPtr new_sp)
{
	CriticalSection _cs_;
	_ASSERT(! m_pause_flg && m_pause_cnt == 0);
		
	m_pending_swc = false;
		
	if ( m_cur_task ) {
#if MACS_USE_CLOCK
		uint32_t duration = System::GetCurCpuTick() -	m_cur_task->m_switch_cpu_tick;
		m_cur_task->m_run_duration.m_frac += duration;
		m_cur_task->m_run_duration.Norm();
#endif	
		m_cur_task->m_stack.m_top = new_sp;

#if MACS_DEBUG		
		bool res = m_cur_task->m_stack.Check();
		if ( ! res ) 
			m_cur_task = nullptr;
#endif		
	}
		
#if MACS_IRQ_FAST_SWITCH		
	if ( m_irq_tasks.NeedIrqActivate() )
		m_irq_tasks.ActivateTasks();	
#endif
		  
	SelectNextTask();
	
#if MACS_MPU_PROTECT_STACK		
	m_cur_task->m_stack.SetMpuMine();	
#endif
	 
	System::SetPrivMode(m_cur_task->m_mode == Task::ModePrivileged);
	
#if MACS_USE_CLOCK
	m_cur_task->m_switch_cpu_tick = System::GetCurCpuTick();
#endif
	
	return m_cur_task->m_stack.m_top;
}

extern "C" bool SchedulerSysTickHandler()
{
	return Sch().SysTickHandler();
}

////////////////////////////////////////////////////////////////
void TaskIrqRoom::ProceedIrq(int irq_num)
{
	for ( TaskIrq * ptsk = m_irq_task_list; ptsk != nullptr; ptsk = TaskIrqList::Next(ptsk) ) 
		if ( ptsk->m_irq_num == irq_num ) {
			if ( ptsk->GetState() == Task::StateBlocked && ! ptsk->m_unblock_func ) 
				m_event = true;
			ptsk->m_irq_up = true;
		}
#if MACS_IRQ_FAST_SWITCH
	if ( m_event && Sch().m_started ) {
		Sch().m_pending_swc = true;
		Sch().Yield();
	}
#endif
}
  
void TaskIrqRoom::ActivateTasks()
{
	for ( TaskIrq * ptsk = m_irq_task_list; ptsk != nullptr; ptsk = TaskIrqList::Next(ptsk) ) 
		if ( ptsk->m_irq_up && ptsk->GetState() == Task::StateBlocked && ! ptsk->m_unblock_func ) {
			Sch().m_sleep_tasks.Remove(ptsk);
			Sch().UnblockTaskInternal(ptsk, Task::UnblockReasonIrq);
			ptsk->m_irq_up = false;
		}
	m_event = false;
}
////////////////////////////////////////////////////////////////
uint32_t Read_Cpu_Tick_Priv() { return System::GetCurCpuTick(); }
 
uint Scheduler::GetTasksQty()
{
	return m_work_tasks.Qty() + m_sleep_tasks.Qty() + (m_cur_task ? 1 : 0); 
}

#if MACS_USE_CLOCK
void TaskInfo::PrintHeader(String & str)	
{
	str.Add("        Task  Pr   Cpu. tm.  Stck a/u");
}
 
void TaskInfo::Print(String & str)	
{
	String prior_str; PrintPriority(prior_str, m_priority);
	str.Add(PrnFmt("%12.12s  %2.2s  %9.9s  %ld/%ld", m_name, prior_str.Z(), m_dur.ToStr(), m_stack_usage, m_stack_len));
}
  
void Scheduler::CollectTasksInfo(DynArr<TaskInfo> & info, Task * task, bool is_list)
{
	while ( task ) {
		TaskInfo tinfo;
		CSPTR tname = task->GetName();
		if ( tname ) {
			strncpy(tinfo.m_name, tname, sizeof(tinfo.m_name));
			tinfo.m_name[sizeof(tinfo.m_name) - 1] = '\0';
		} 
		tinfo.m_priority = task->m_priority;
		tinfo.m_dur = task->m_run_duration;
		
		tinfo.m_stack_len = task->GetStackLen();
		tinfo.m_stack_usage = task->GetStackUsage();
		
		info.Add(tinfo);
		if ( ! is_list )
			return;
		task = task->m_next_sched_task;
	}
}
 
Result Scheduler::GetTasksInfo(DynArr<TaskInfo> & info)
{
	PauseSection _ps_;
	
	uint tqty = GetTasksQty();
	info.Clear();
	info.SetCapacity(tqty);
	
	CollectTasksInfo(info, m_cur_task, false);
	CollectTasksInfo(info, m_work_tasks.FirstTask(), true);
	CollectTasksInfo(info, m_sleep_tasks.FirstTask(), true);

	return ResultOk;
}
#endif
////////////////////////////////////////////////////////////////
void TaskWorkRoom::Insert(Task * task)
{
	_ASSERT(! Sch().m_sleep_tasks.IsInList(task));

	TaskWorkList::Add(m_task_list, task);		
}

void TaskSleepRoom::Insert(Task * task)
{
	_ASSERT(task->m_dream_ticks);
	_ASSERT(! Sch().m_work_tasks.IsInList(task));
	
	TaskSleepList::Add(m_task_list, task);		
}
 
void TaskSleepRoom::Tick()
{
	for ( Task * ptsk = m_task_list; ptsk != nullptr; ptsk = TaskSleepList::Next(ptsk) ) {
		_ASSERT(ptsk->m_dream_ticks);
		if ( ptsk->m_dream_ticks != ULONG_MAX )
			-- ptsk->m_dream_ticks;
	}
}

}	// namespace macs
