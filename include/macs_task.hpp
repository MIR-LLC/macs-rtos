/// @file macs_task.hpp
/// @brief Задачи.
/// @details Предоставляет базовые классы для реализации задач. Любая задача в приложении
/// должна быть унаследована от соответствующего базового класса. В унаследованном классе
/// следует переопределить виртуальный метод, содержащий код задачи.
/// @copyright AstroSoft Ltd, 2016
 
#pragma once
 
#include <stdint.h>
#include <stddef.h>

#include "macs_system.hpp"
#include "macs_common.hpp"
#include "macs_list.hpp"
#if MACS_USE_CLOCK
	#include "macs_clock.hpp"
#endif
  
namespace macs {
	 
class Scheduler;
class SyncObject;
class SyncOwnedObject;
class Event;
class Mutex;
class Semaphore;

/// @brief Базовый класс для представления задачи.
/// @details Представляет собой задачу (в "больших" ОС обычно используют термин "поток").
/// Для создания пользовательской задачи необходимо создать производный класс
/// и переопределить метод Execute().
/// Если нужны параметры, то их можно добавить в конструктор. 
/// Также есть параметр, присутствующий у всех задач - имя, используется только для отладки.
class Task
{
public:
	/// @brief Расширенные приоритеты задач. В принципе могут иметь любое значение (максимум задается через MACS_MAX_TASK_PRIORITY). 
	/// @details Результат операции PriorityMax + 1 равен PriorityMax.
	enum Priority
	{
		PriorityIdle = 0,            ///< Низший приоритет (установлен для задачи IDLE)			
		PriorityLow = 10,            ///< Низкий приоритет			
		PriorityBelowNormal = 20,    ///< Приоритет ниже обычного
		PriorityNormal = 30,         ///< Обычный приоритет (используется по умолчанию)
		PriorityAboveNormal = 40,    ///< Приоритет выше обычного
		PriorityHigh = 50,           ///< Высокий приоритет
		PriorityRealtime = 60,       ///< Наивысший приоритет (реального времени)
			 
		PriorityMax = MACS_MAX_TASK_PRIORITY, 
		
		PriorityInvalid = MACS_MAX_TASK_PRIORITY + 1  
	};

	/// @brief Состояния задач.
	enum State
	{
		/// Задача готова к выполнению
		StateReady,

		/// Задача выполняется
		StateRunning,

		/// Задача заблокирована, ждёт события синхронизации или
		/// наступления таймаута
		StateBlocked,

		/// Задача ещё не добавлена в планировщик или уже удалена
		StateInactive
	};

	/// @brief Режим работы процессора для задачи.
	enum Mode
	{
		/// Привилегированный режим
		ModePrivileged,

		/// Непривилегерованный режим
		ModeUnprivileged
	};

	// Причины разблокировки задач.
	enum UnblockReason
	{
		// Не задано
		UnblockReasonNone,

		// Задача была разблокирована при наступлении события синхронизации
		UnblockReasonRequest,

		// Задача была разблокирована при наступлении таймаута
		UnblockReasonTimeout,
			
		// Задача была разблокирована при возникновении прерывания
		UnblockReasonIrq
	};

	// Используется для выполнения пользовательского кода во время разблокировки задачи ядром.
	class UnblockFunctor
	{
	public:
		virtual void OnUnblockTask(Task * task, Task::UnblockReason reason) = 0;
		virtual void OnDeleteTask(Task * task) {}
	};
public:
	/// @brief Минимальный размер стека в словах.
	static const size_t MIN_STACK_SIZE = TaskStack::MIN_SIZE;
	/// @brief Размер стека, достаточный для работы небольших задач
	static const size_t SMALL_STACK_SIZE = (TaskStack::ENOUGH_SIZE + TaskStack::MIN_SIZE) / 2;
	/// @brief Размер стека, достаточный для работы большинства задач
	static const size_t ENOUGH_STACK_SIZE = TaskStack::ENOUGH_SIZE;
	/// @brief  Максимальный размер стека в словах.
	static const size_t MAX_STACK_SIZE = TaskStack::MAX_SIZE;
	  
public:		
	virtual ~Task();
	 
	/// @brief Получить имя задачи.
	/// @return Имя задачи, указанное при создании
	const char * GetName() const { 		
#if MACS_TASK_NAME_LENGTH > 0	 
		return m_name_arr;
#elif MACS_TASK_NAME_LENGTH == -1	  	
		return m_name_ptr;
#else		
		return nullptr; 	
#endif		
	}

	/// @brief Получить текущее состояние задачи.
	/// @return [Состояние задачи](@ref macs::Task::State)
	State GetState() const { return m_state; }

	/// @brief Добавить задачу в планировщик с возможностью указания всех параметров.
	/// @details Если планировщик работает в режиме вытесняющей многозадачности, при выполнении 
	/// этого метода происходит немедленное переключение контекста (возможно, на добавленную задачу, 
	/// если нет более приоритетных).
	/// @param task - указатель на объект-задачу
	/// @param priority - приоритет задачи
	/// @param mode - режим выполнения
	/// @param stack_size - размер стека задачи, по умолчанию Task::ENOUGH_STACK_SIZE
	/// @return [Результат операции](@ref macs::Result)
	static Result Add(Task * task, Task::Priority priority, Task::Mode mode, size_t stack_size = Task::ENOUGH_STACK_SIZE);
	
	/// @brief Добавить задачу в планировщик с возможностью указания всех параметров.
	/// @details Если планировщик работает в режиме вытесняющей многозадачности, при выполнении 
	/// этого метода происходит немедленное переключение контекста (возможно, на добавленную задачу, 
	/// если нет более приоритетных).
	/// @param task - указатель на объект-задачу
	/// @param mode - режим выполнения
	/// @param priority - приоритет задачи
	/// @param stack_size - размер стека задачи, по умолчанию Task::ENOUGH_STACK_SIZE
	/// @return [Результат операции](@ref macs::Result)
	static inline Result Add(Task * task, Task::Mode mode, Task::Priority priority, size_t stack_size = Task::ENOUGH_STACK_SIZE) {
		return Add(task, priority, mode, stack_size);
	}
	
	/// @brief Добавить задачу в планировщик с возможностью задать размер стека.
	/// @details Добавляет задачу на исполнение в непривилегированном режиме (Task::ModeUnprivileged)
	/// и с обычным приоритетом (Task::PriorityNormal).
	/// @param task - указатель на объект-задачу
	/// @param stack_size - размер стека задачи, по умолчанию Task::ENOUGH_STACK_SIZE
	/// @return [Результат операции](@ref macs::Result)
	static inline Result Add(Task * task, size_t stack_size = Task::ENOUGH_STACK_SIZE) {
		return Add(task, Task::PriorityNormal, Task::ModeUnprivileged, stack_size);
	}

	/// @brief Добавить задачу в планировщик с указанием приоритета и возможностью задать размер стека.
	/// @details Добавляет задачу на исполнение в непривилегированном режиме (Task::ModeUnprivileged).
	/// @param task - указатель на объект-задачу
	/// @param priority - приоритет задачи
	/// @param stack_size - размер стека задачи, по умолчанию Task::ENOUGH_STACK_SIZE
	/// @return [Результат операции](@ref macs::Result)
	static inline Result Add(Task * task, Task::Priority priority, size_t stack_size = Task::ENOUGH_STACK_SIZE) {
		return Add(task, priority, Task::ModeUnprivileged, stack_size);
	}

	/// @brief Добавить задачу в планировщик с указанием режима выполнения и возможностью задать размер стека.
	/// @details Добавляет задачу на исполнение с обычным приоритетом (Task::PriorityNormal).
	/// @param task - указатель на объект-задачу
	/// @param mode - режим выполнения
	/// @param stack_size - размер стека задачи, по умолчанию Task::ENOUGH_STACK_SIZE
	/// @return [Результат операции](@ref macs::Result)
	static inline Result Add(Task * task, Task::Mode mode, size_t stack_size = Task::ENOUGH_STACK_SIZE) {
		return Add(task, Task::PriorityNormal, mode, stack_size);
	}

	/// @brief Удалить данную задачу без уничтожения объекта.
	/// @details Удаляет задачу из планировщика. Если задача удаляет сама себя, происходит 
	/// немедленное переключение контекста.
	/// @return [Результат операции](@ref macs::Result)
	Result Remove();

	/// @brief Удалить данную задачу с освобождением памяти объекта.
	/// @details Удаляет задачу из планировщика. Если задача удаляет сама себя, происходит 
	/// немедленное переключение контекста.
	/// @return [Результат операции](@ref macs::Result)
	Result Delete();
	
	/// @brief Выполнить задержку, блокируя ТЕКУЩУЮ (с точки зрения планировщика) задачу. 
	/// @details Метод выполняет задержку, блокируя исполняемую в текущий момент задачу.
	/// Следует учитывать это обстоятельство при записи типа task->Delay();
	/// @param timeout_ms - значение задержки в миллисекундах.
	/// @return [Результат операции](@ref macs::Result)
	static Result Delay(uint32_t timeout_ms);

	/// @brief Выполнить задержку, не блокируя задачу, но расходуя процессорное время.
	/// @details Метод может быть использован там где переключение контекста не нужно или невозможно, 
	/// например при кооперативной многозадачности или отладке.
	/// @param timeout_ms - значение задержки в миллисекундах.
	static void CpuDelay(uint32_t timeout_ms);

	/// @brief Получить приоритет задачи.
	/// @return [Приоритет задачи](@ref macs::Task::Priority)
	inline Priority GetPriority() const { return m_priority; }

	/// @brief Устанавить приоритет задачи.
	/// @details Устанавливает приоритет данной задачи. При вытесняющей многозадачности следствием может 
	/// быть немедленное переключение контекста в данной точке, не дожидаясь окончания текущего кванта времени.
	/// @param value - новый приоритет задачи.
	/// @return [Результат операции](@ref macs::Result)
	Result SetPriority(Priority value);

	/// @brief Получить текущую выполняемую задачу.
	/// @return указатель на текущую задачу
	static Task * GetCurrent();

	/// @brief Выполнить \[принудительное\] переключение контекста.
	/// @details Используется для передачи управления следующей задаче. В первую очередь, 
	/// данная функция важна при кооперативной многозадачности, так как в этом случае
	/// нет переключений по квантам времени. Но и при вытесняющей многозадачности, если известно, 
	/// что задача пока больше не нужна, она может отдать остаток кванта времени другим задачам, вызвав данную функцию.
	static void Yield();

    /// @brief Получить размер стека задачи.
    /// @details Возвращает размер стека данной задачи в словах, указанный при создании задачи
    /// или её добавлении в планировщик. Если задача была создана со стеком в динамической памяти, но
    /// ещё не добавлена в планировщик, то возвращается 0.
    /// @return Размер стека задачи.
	inline size_t GetStackLen()		const { return m_stack.GetLen(); }

    /// @brief Получить максимальный размер использования стека задачи.
    /// @details Возвращает максимальное количество слов, занятых в памяти стека задачи с момента
    /// последнего инструментирования до вызова данной функции (при включенной опции @ref MACS_WATCH_STACK
    /// инструментирование стека выполняется автоматически при добавлении задачи в планировщик).
    /// Стек задачи должен быть предварительно инструментирован (см. @ref Task::InstrumentStack()).
    /// Результат вызова функции на неинструменированном стеке не определён.
    /// Вызов функции запрещён в случае, если задача ещё не добавлена в планировщик (может привести к
    /// непредсказуемому поведению).
    /// @return Максимальный размер занятой памяти в стеке задачи.
	inline size_t GetStackUsage() const { return m_stack.GetUsage(); }

    /// @brief Инструментировать стек задачи.
    /// @details Инструментирование необходимо для определения глубины использования стека задачи,
    /// поэтому вызов данной функции должен предшествовать вызову функции @ref Task::GetStackUsage.
    /// Данная функция выполняет инструментирование свободной на момент вызова части стека задачи.
    /// С включенной опцией @ref MACS_WATCH_STACK инструментирование стека выполняется автоматически
    /// при добавлении задачи в планировщик, т.е. во время старта задачи стек будет полностью
    /// инструментирован. Последующий вызов данной функции заново инструментирует свободную
    /// часть стека, что может быть необходимо, например, для определения глубины использования
    /// стека при выполнении секции кода.
	void InstrumentStack() { m_stack.Instrument(); }
	 
protected:
	/// @brief Конструктор задачи со стеком в динамической памяти.
	/// @details Рекомендуется вызывать этот конструктор в конструкторе пользовательской
	/// задачи. Задача создается с обычным приоритетом и непривилегированным режимом
	/// выполнения (но если определен символ препроцессора MACS_PROFILING_ENABLED, то с
	/// привилегированным режимом выполнения). Эти параметры можно изменить в 
	/// собственном конструкторе либо при добавлении задачи в планировщик (метод Add()).
	/// @param name - имя задачи (опционально, может использоваться для отладки)
	Task(const char * name = nullptr) 
		{ Init(name, 0, nullptr); }

/// @brief Конструктор задачи со стеком во внешней памяти.
	/// @details Рекомендуется вызывать этот конструктор в конструкторе пользовательской
	/// задачи. Задача создается с обычным приоритетом и непривилегированным режимом
	/// выполнения (но если определен символ препроцессора MACS_PROFILING_ENABLED, то с
	/// привилегированным режимом выполнения). Эти параметры можно изменить в 
	/// собственном конструкторе либо при добавлении задачи в планировщик (метод Add()).
	/// @param stack_len - размер стека задачи (в словах)
	/// @param stack_mem - указатель на начало буфера для стека задачи
	/// @param name - имя задачи (опционально, может использоваться для отладки)
	Task(size_t stack_len, uint32_t * stack_mem, const char * name = nullptr)
		{ Init(name, stack_len, stack_mem); }

private:	
	CLS_COPY(Task)

	void Init(const char * name, size_t stack_len, uint32_t * stack_mem);

	// инициализирует стек, вызывается ядром 
	void InitializeStack(size_t stack_len, void (* onTaskExit)(void));

	bool IsRunnable() const { return m_state == Task::StateRunning || m_state == Task::StateReady; }

	/// @brief Выполняет задачу.
	/// @details Реализация рабочих функций задачи. Вызывается ядром. Данный метод должен быть переопределен в
	/// в пользовательском классе задачи.
	virtual void Execute() = 0;
public:
	static void Execute_(Task * task) {
		task->Execute(); 
		task->Remove();
	}
private:
	inline uint32_t GetExecuteAddress() { return (uint32_t) & Task::Execute_; }
 
	void SetBlockSync(SyncObject *);            // устанавливает ссылку в задаче на блокирующий объект синхронизации
	void AddOwnedSync(SyncOwnedObject *);       // добавляет ссылку в задаче на принадлежащий объект синхронизации
	void DropBlockSync(SyncObject *);           // снимает ссылку в задаче на блокирующий объект синхронизации
	void RemoveOwnedSync(SyncOwnedObject *);    // убирает ссылку из задачи на принадлежащий объект синхронизации
	void DetachFromSync();                      // убирает ссылки из всех объектов синхронизации
		
	friend class Scheduler;
	friend class SyncObject;
	friend class Mutex;
	friend class Semaphore;
	friend class Event;
	friend class TaskRoom;
	friend class TaskSleepRoom;
	friend class TaskIrqRoom;
	friend Result DeleteTask_Priv(Scheduler * scheduler, Task * task, bool del_mem);
	friend Result BlockCurrentTask_Priv(Scheduler * scheduler, uint32_t timeout_ms, Task::UnblockFunctor *);
	friend Result UnblockTask_Priv(Scheduler * scheduler, Task * task);
#if MACS_MUTEX_PRIORITY_INVERSION
	friend Result IntSetTaskPriority_Priv(Scheduler * scheduler, Task * task, Task::Priority priority, bool internal_usage);
#else
	friend Result SetTaskPriority_Priv(Scheduler * scheduler, Task * task, Task::Priority priority);
#endif
	friend void _SetTaskPriority_Priv(Scheduler * scheduler, Task * task, Task::Priority priority);
	friend bool PriorPreceeding(Task * task_a, Task * task_b);
	friend bool WakeupPreceeding(Task * task_a, Task * task_b);
	 
	// имя задачи
#if MACS_TASK_NAME_LENGTH > 0	 
	char 				 m_name_arr[MACS_TASK_NAME_LENGTH + 1];
#elif MACS_TASK_NAME_LENGTH == -1	  	
	const char * m_name_ptr;
#endif	

public:
	TaskStack m_stack;
	 
private:
	Priority m_priority;
	State m_state;
	Mode m_mode;
	 
#if MACS_USE_CLOCK
	Time     m_run_duration;
	uint32_t m_switch_cpu_tick;
#endif	
	 
	uint32_t m_dream_ticks;		// на сколько тиков усыпили задачу
public:
	Task * m_next_sched_task;	// Следующая задача в списке планировщика (work или sleep)
	Task * m_next_sync_task;	// Следующая заблокированная задача в списке объекта синхронизации
private:		
	UnblockFunctor * m_unblock_func;	// содержит функтор, который будет выполнен во время разблокировки задачи ядром
	SyncOwnedObject * m_owned_obj_list;	// Список принадлежащих задаче объектов синхронизации

	// содержит причину последней разблокировки
	UnblockReason m_unblock_reason;
};

inline bool PriorPreceeding(Task * task_a, Task * task_b)
{
	return task_a->m_priority > task_b->m_priority;
}
inline bool WakeupPreceeding(Task * task_a, Task * task_b)
{
	return task_a->m_dream_ticks <= task_b->m_dream_ticks;
}
 
SLISTORD_DECLARE(TaskSyncList, Task, m_next_sync_task, PriorPreceeding);
SLIST_DECLARE(TaskRoomList, Task, m_next_sched_task);
SLISTORD_DECLARE(TaskWorkList, Task, m_next_sched_task, PriorPreceeding);
SLISTORD_DECLARE(TaskSleepList, Task, m_next_sched_task, WakeupPreceeding);
	
inline Task::Priority operator + (const Task::Priority prior, const int chg)
{
	Task::Priority res = (Task::Priority) ((int) prior + chg);
	return res <= Task::PriorityMax ? res : Task::PriorityMax;
}
inline Task::Priority operator - (const Task::Priority prior, const int chg)
{
	return prior + (-chg);
}
  
inline Task::Priority RandPriority(const Task::Priority max_prior = Task::PriorityMax, const Task::Priority min_prior = Task::PriorityIdle + 1)
{ 
	_ASSERT(max_prior > min_prior);
	return min_prior + RandN(max_prior - min_prior - 1);
}
 
extern void PrintPriority(String & str, Task::Priority prior, bool brief = true);

/// @brief Класс для задачи, которая не содержит собственных данных.
/// @details Данная задача позволяет не создавать класс,
/// а ограничиться указанием функции-обработчика, которая будет вызвана из метода Execute:	
/// void f(Task *) { ++ i; } 
/// Task::Add(new TaskNaked(f));
class TaskNaked : public Task
{
private:
	void (* m_exec_func)(Task *);

public:
	/// @brief Конструктор задачи.
	/// @details Создается задачу, которая выполняет указанную функцию.
	/// @param exec_func - указатель на функцию, которую нужно выполнить в создаваемой задаче
	/// @param name - имя задачи (для отладки)
	TaskNaked(void (* exec_func)(Task *), const char * name = nullptr) : Task(name) { 
		_ASSERT(exec_func);
		m_exec_func = exec_func; 
	}
	  
	virtual void Execute() { 
		(* m_exec_func)(this);
	}
};
	
/// @brief Класс для задачи, которая служит обработчиком прерывания.
/// @details Данная задача отличается от обычной тем, что после добавления в планировщик
/// остается неактивной до тех пор, пока не произойдет заданное прерывание.
///	Вместо метода Execute вызывается (однократно) метод IrqHandler.
class TaskIrq : public Task
{
private:
	bool m_irq_up;           // Произошло прерывание 
public:
	TaskIrq * m_next_irq_task;	// Следующая задача-обработчик

protected:
	int m_irq_num;           // Номер обслуживаемого прерывания

	/// @brief Конструктор задачи-обработчика.
	/// @details Создается задача с указанным именем.
	/// @param name - имя задачи (для отладки)
	TaskIrq(const char * name = nullptr);
	 
public:
	/// @brief Добавить задачу-обработчик в планировщик с указанием всех параметров.
	/// @details Отличается от функции класса Task параметром irq_num,
	/// который задает прерывание в стиле IRQn_Type.
	/// @param task - указатель на объект-задачу
	/// @param irq_num - номер прерывания, которому будет назначен обработчик
	/// @param priority - приоритет задачи
	/// @param mode - режим выполнения
	/// @param stack_size - размер стека задачи, по умолчанию Task::ENOUGH_STACK_SIZE
	/// @return [Результат операции](@ref macs::Result)
	static Result Add(TaskIrq * task, int irq_num, Task::Priority priority, Task::Mode mode, size_t stack_size = Task::ENOUGH_STACK_SIZE);

	/// @brief Добавить задачу-обработчик в планировщик с указанием всех параметров.
	/// @details Отличается от функции класса Task параметром irq_num,
	/// который задает прерывание в стиле IRQn_Type.
	/// @param task - указатель на объект-задачу
	/// @param irq_num - номер прерывания, которому будет назначен обработчик
	/// @param mode - режим выполнения
	/// @param priority - приоритет задачи
	/// @param stack_size - размер стека задачи, по умолчанию Task::ENOUGH_STACK_SIZE
	/// @return [Результат операции](@ref macs::Result)
	static inline Result Add(TaskIrq * task, int irq_num, Task::Mode mode, Task::Priority priority, size_t stack_size = Task::ENOUGH_STACK_SIZE) {
			return Add(task, irq_num, priority, mode, stack_size);
	}

	/// @brief Добавить задачу-обработчик в планировщик с возможностью задать размер стека.
	/// @details Отличается от функции класса Task параметром irq_num,
	/// который задает прерывание в стиле IRQn_Type.
	/// @param task - указатель на объект-задачу
	/// @param irq_num - номер прерывания, которому будет назначен обработчик
	/// @param stack_size - размер стека задачи, по умолчанию Task::ENOUGH_STACK_SIZE
	/// @return [Результат операции](@ref macs::Result)
	static inline Result Add(TaskIrq * task, int irq_num, size_t stack_size = Task::ENOUGH_STACK_SIZE) {
		return Add(task, irq_num, Task::PriorityNormal, Task::ModeUnprivileged, stack_size);
	}
		
	/// @brief Добавить задачу-обработчик в планировщик с указанием приоритета и возможностью задать размер стека.
	/// @details Отличается от функции класса Task параметром irq_num,
	/// который задает прерывание в стиле IRQn_Type.
	/// @param task - указатель на объект-задачу
	/// @param irq_num - номер прерывания, которому будет назначен обработчик
	/// @param priority - приоритет задачи
	/// @param stack_size - размер стека задачи, по умолчанию Task::ENOUGH_STACK_SIZE
	/// @return [Результат операции](@ref macs::Result)
	static inline Result Add(TaskIrq * task, int irq_num, Task::Priority priority, size_t stack_size = Task::ENOUGH_STACK_SIZE) {
		return Add(task, irq_num, priority, Task::ModeUnprivileged, stack_size);
	}
		
	/// @brief Добавить задачу-обработчик в планировщик с указанием режима выполнения и возможностью задать размер стека.
	/// @details Отличается от функции класса Task параметром irq_num,
	/// который задает прерывание в стиле IRQn_Type.
	/// @param task - указатель на объект-задачу
	/// @param irq_num - номер прерывания, которому будет назначен обработчик
	/// @param mode - режим выполнения
	/// @param stack_size - размер стека задачи, по умолчанию Task::ENOUGH_STACK_SIZE
	/// @return [Результат операции](@ref macs::Result)
	static inline Result Add(TaskIrq * task, int irq_num, Task::Mode mode, size_t stack_size = Task::ENOUGH_STACK_SIZE) {
		return Add(task, irq_num, Task::PriorityNormal, mode, stack_size);
	}

	/// @brief Сделать настройки (возможно, платформозависимые), позволяющие использовать задачу-обработчик данного прерывания
	/// @details Можно вызывать как до, так и после добавления задачи-обработчика в планировщик. Можно вызывать несколько раз, настраивая параметры по очереди.
	/// @param irq_num - номер прерывания, которому будет или уже назначен обработчик
	/// @param vector - истина, если нужно настраивать вектор прерывания
	/// @param enable - истина, если нужно разрешать данное прерывание сейчас
	/// @return истина, если запрошенные настройки были сделаны, ложь, если их почему-либо нельзя сделать
	static inline bool SetUp(int irq_num, bool vector = true, bool enable = true) {
		return System::SetUpIrqHandling(irq_num, vector, enable);
	}

	/// @brief Метод для обработки прерывания.
	/// @details Вызывается ядром однократно на каждое случившееся прерывание.
	/// Должен быть определен в производном классе.
	virtual void IrqHandler() = 0;
private:
	/// @brief В потомках TaskIrq данный метод не следует переопределять! 
	virtual void Execute();
	
	friend class TaskRoom;
	friend class TaskIrqRoom;
	friend class Scheduler;
	friend Result AddTaskIrq_Priv(Scheduler * scheduler, TaskIrq * task);
};
  
class SyncObject : public Task::UnblockFunctor 
{
public:
	Task * m_blocked_task_list;
public:
	SyncObject() { m_blocked_task_list = nullptr;	}
		
	inline bool IsHolding() const { return !! m_blocked_task_list; }

	virtual Result BlockCurTask(uint32_t timeout_ms);
	virtual Result UnblockTask();
	virtual void OnUnblockTask(Task *, Task::UnblockReason);
	virtual void OnDeleteTask(Task *);

protected:
	void DropLinks();
};
 
class SyncOwnedObject : public SyncObject
{
public:
	Task * m_owner;
	Task::Priority m_owner_original_priority;	
	SyncOwnedObject * m_next_owned_obj;
public:
	SyncOwnedObject() { m_owner = nullptr; m_next_owned_obj = nullptr; }	
};		 
SLIST_DECLARE(OwnedSyncObjList, SyncOwnedObject, m_next_owned_obj); 

}	// namespace macs
