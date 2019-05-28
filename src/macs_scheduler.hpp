/// @file macs_scheduler.hpp
/// @brief Планировщик.
/// @details Содержит описание планировщика и связанного с ним функционала.
/// @copyright AstroSoft Ltd, 2016
 
#pragma once

#include <stdint.h>
#include <stddef.h>

#include "macs_common.hpp"
#include "macs_task.hpp"
#include "macs_critical_section.hpp"

namespace macs {

extern "C"
{
	extern bool SchedulerSysTickHandler();	
	extern StackPtr SchedulerSwitchContext(StackPtr new_sp);
	extern void MacsIrqHandler();
}

class TaskRoom
{
public:
	Task * m_task_list;
public:
	TaskRoom() { m_task_list = nullptr; }
	      Task * FirstTask()       { return m_task_list; }
	const Task * FirstTask() const { return m_task_list; }
	ulong Qty() const { return TaskRoomList::Qty(m_task_list); }
#if MACS_DEBUG	
	inline bool IsInList(Task * task) { return !! * TaskRoomList::Find(m_task_list, task); }
#endif	
	inline void Remove(Task * task) { TaskRoomList::Del(m_task_list, task); }
};

class TaskSleepRoom : public TaskRoom 
{
public:
	inline void Insert(Task * task);
	inline Task * Fetch() 
	{
		return (m_task_list && ! m_task_list->m_dream_ticks) ? TaskSleepList::Fetch(m_task_list) : nullptr;
	}
	
	void Tick();
};

class TaskWorkRoom : public TaskRoom 
{
public:
	inline void Insert(Task * task);
	inline Task * Fetch() { return TaskWorkList::Fetch(m_task_list); }
};

SLIST_DECLARE(TaskIrqList, TaskIrq, m_next_irq_task); 
class TaskIrqRoom 
{
private:
	TaskIrq * m_irq_task_list;
	bool m_event;	// Необходима обработка события
public:
	TaskIrqRoom() { m_event = false; m_irq_task_list = nullptr; }
	
	inline void Add(TaskIrq * task) { TaskIrqList::Add(m_irq_task_list, task); }
	inline void Del(TaskIrq * task) { TaskIrqList::Del(m_irq_task_list, task); }
	
	void ProceedIrq(int irq_num);

	void ActivateTasks();
	bool NeedIrqActivate() { return m_event; }
}; 
 
#if MACS_USE_CLOCK
class TaskInfo
{
public:
	char m_name[12];
	Task::Priority m_priority;
	Time m_dur;
	size_t m_stack_len, m_stack_usage;
public:
	TaskInfo() { m_name[0] = '\0'; m_priority = Task::PriorityInvalid; m_dur.Zero(); m_stack_len = m_stack_usage = 0; }
	static void PrintHeader(String &);
	void Print(String &);	
};
#endif	
 
/// @brief Планировщик задач.
/// @details Реализует механизмы вытесняющей многозадачности по принципу round robin,
/// а также кооперативной многозадачности. Многие публичные методы планировщика
/// дублируют одноименные методы класса Task. Данные методы не документируются. При программировании приложений
/// рекомендуется использовать методы класса Task.
class Scheduler
{
public:
	/// @brief Получить (единственный) экземляр данного класса
	/// @return Ссылка на объект планировщика
	static inline Scheduler & GetInstance() { return m_instance; }

	// Инициализирует планировщик и запускает таймер отсчёта времени.
	// Отделена от функции Start для того чтобы была возможность использовать GetTickCount
	// до старта планировщика, например для инициализации драйверов.
	Result Initialize();
	 
	/// @brief Проверка инициализации планировщика.
	/// @details Возвращает истину, если планировщи был инициализирован.
	inline bool IsInitialized() { return m_initialized; }

	// Запускает планировщик и начинает выполнение первой задачи.
	// Если параметр usePreemption = true, то будет использоваться вытесняющая многозадачность,
	// иначе - кооперативная.
	Result Start(bool use_preemption = true);

	/// @brief Проверка старта планировщика.
	/// @details Возвращает истину, если планировщи был запущен.
	inline bool IsStarted() { return m_started; } 
	
	/// @brief Получить количество системных тиков
	/// @return количество тиков, прошедшее с момента вызова Initialize.
	uint32_t GetTickCount() const { return m_tick_count; }

	/// @brief Удаляет задачу из планировщика, но не освобождает выделенную под неё память.
	/// @details Если задача удаляет себя, то выполняется немедленное переключение контекста.
	/// @return [Результат операции](@ref macs::Result) 
	Result RemoveTask(Task * task) { return DeleteTask(task, false); }

	/// @brief Удаляет задачу из планировщика и освобождает память объекта.
	/// @details Если задача удаляет себя, то выполняется немедленное переключение контекста.
	/// @return [Результат операции](@ref macs::Result)
	Result DeleteTask(Task * task) { return DeleteTask(task, true); }
	
	// Блокирует задачу, которая выполняется в данный момент и выполняет немедленное переключение контекста.
	Result BlockCurrentTask(uint32_t timeout_ms = INFINITE_TIMEOUT, Task::UnblockFunctor * unblock_functor = nullptr);

	// Разблокирует указанную задачу. Если разблокированная задача имеет более высокий приоритет, чем
	// текущая задача и, если используется вытесняющая многозадачность, то будет выполнено переключение контекста.
	Result UnblockTask(Task * task);

	// Устанавливает приоритет указанной задачи и, если используется вытесняющая многозадачность,
	// выполняет переключение контекста.
	Result SetTaskPriority(Task * task, Task::Priority priority);

	// Возвращает текущую выполняемую задачу.
	inline Task * GetCurrentTask() const { return m_cur_task; }

	// Выполняет переключение контекста.
	inline void Yield() {
		if ( ! m_started )
			return;

		System::IsInPrivOrIrq() ? Yield_Priv(this)
		                        :	(void) SvcExecPrivileged(this, NULL, NULL, EPM_Yield_Priv);
	}
		
	/// @brief Обработать прерывание
	/// @details Вызывается системой при возникновении реального прерывания и пользователем для виртуального.
	/// @param irq_num - номер прерывания
	void ProceedIrq(int irq_num) { m_irq_tasks.ProceedIrq(irq_num); }
 
	/// @brief Приостановить переключение задач.
	/// @details Допускаются вложенные вызовы этого метода.
	/// @param set_on - флаг паузы (true - включить паузу, false - отключить паузу)
	/// @return [Результат операции](@ref macs::Result)
	Result Pause(bool set_on);	

	/// @brief Получить количество задач
	/// @details Возвращает количество задач в планировщике (готовых к выполнению и
	/// заблокированных) на момент вызова функции.
	/// @return Количество задач в планировщике
	uint GetTasksQty();

#if MACS_USE_CLOCK
	Result GetTasksInfo(DynArr<TaskInfo> &);
#endif	

private:
	Scheduler();
	~Scheduler();

	CLS_COPY(Scheduler)

	bool UnblockTaskInternal(Task * task, Task::UnblockReason reason);
	void ForceContextSwitch();
	void SelectNextTask();
public:
	StackPtr SwitchContext(StackPtr new_sp);
	inline void TryContextSwitch() { // Только для вызова из критической секции!
		if ( ! m_pause_flg && m_pause_cnt == 0 )
			System::SwitchContext();
		else 
			m_pending_swc = true;
	}
private:
	bool SysTickHandler();
	bool IsContextSwitchRequired();
	bool IsPriorityValid(Task::Priority priority);
	void TuneProfiler();
#if MACS_USE_CLOCK
	void CollectTasksInfo(DynArr<TaskInfo> &, Task *, bool is_list);
#endif

	// Добавляет задачу в планировщик и выполняет переключение контекста.
	Result AddTask(Task * task, Task::Priority priority = Task::PriorityNormal,
								 Task::Mode mode = Task::ModeUnprivileged, size_t stack_size = Task::MIN_STACK_SIZE);
	Result AddTask(TaskIrq * task, int irq_num, Task::Priority priority, Task::Mode mode, size_t stack_size);
	Result DeleteTask(Task * task, bool del_mem);

	friend StackPtr SchedulerSwitchContext(StackPtr new_sp);
	friend bool SchedulerSysTickHandler();
	friend void Yield_Priv(Scheduler * scheduler);
	friend Result AddTask_Priv(Scheduler * scheduler, Task * task);  
	friend Result AddTaskIrq_Priv(Scheduler * scheduler, TaskIrq * task); 
	friend Result BlockCurrentTask_Priv(Scheduler * scheduler, uint32_t timeout_ms, Task::UnblockFunctor *); 
	friend Result DeleteTask_Priv(Scheduler * scheduler, Task * task, bool del_mem);
	friend Result UnblockTask_Priv(Scheduler * scheduler, Task * task);
#if MACS_MUTEX_PRIORITY_INVERSION
	friend Result IntSetTaskPriority_Priv(Scheduler * scheduler, Task * task, Task::Priority priority, bool internal_usage);
#else
	friend Result SetTaskPriority_Priv(Scheduler * scheduler, Task * task, Task::Priority priority);
#endif
	friend void _SetTaskPriority_Priv(Scheduler * scheduler, Task * task, Task::Priority priority);
	friend class Task;
	friend class TaskIrq;
	friend class TaskIrqRoom;
#if MACS_DEBUG
	friend class TaskSleepRoom;
	friend class TaskWorkRoom;
#endif		
	friend class PauseSection;
	friend void MacsIrqHandler();

	static Scheduler m_instance;

	TaskSleepRoom m_sleep_tasks;
	TaskWorkRoom	m_work_tasks;
	TaskIrqRoom 	m_irq_tasks;

	Task * m_cur_task;	// Текущая задача хранится только здесь - в списке ее нет!
	volatile uint32_t m_tick_count;

	bool m_initialized;
	bool m_started;
	bool m_pause_flg;
	uint m_pause_cnt;
	bool m_pending_swc;
	bool m_use_preemption;
};	//  class Scheduler

inline Scheduler & Sch() { return Scheduler::GetInstance(); }

class PauseSection
{
public:
	PauseSection()  { Sch().Pause(true);  }
	~PauseSection() { Sch().Pause(false); }
};

extern Result AddTask_Priv(Scheduler * scheduler, Task * task);
extern Result AddTaskIrq_Priv(Scheduler * scheduler, TaskIrq * task);
extern void Yield_Priv(Scheduler * scheduler);
extern Result DeleteTask_Priv(Scheduler * scheduler, Task * task, bool del_mem);
extern Result UnblockTask_Priv(Scheduler * scheduler, Task * task);
#if MACS_MUTEX_PRIORITY_INVERSION
extern Result InfSetTaskPriority_Priv(Scheduler * scheduler, Task * task, Task::Priority priority, bool internal_usage);
#endif
extern Result SetTaskPriority_Priv(Scheduler * scheduler, Task * task, Task::Priority priority);
extern void _SetTaskPriority_Priv(Scheduler * scheduler, Task * task, Task::Priority priority);
extern Result BlockCurrentTask_Priv(Scheduler * scheduler, uint32_t timeout_ms, Task::UnblockFunctor *);
extern uint32_t Read_Cpu_Tick_Priv();

}	// namespace macs

extern Result Spi_Initialize_Priv(void * spi);
extern void Spi_PowerControl(void * spi, uint32_t state);
