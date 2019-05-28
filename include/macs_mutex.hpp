/// @file macs_mutex.hpp
/// @brief Мьютексы.
/// @details Мьютекс (MUTualEXclusive) – это объект синхронизации, у которого в каждый
/// момент времени может быть только один владелец. Все остальные задачи, пытающиеся
/// захватить мьютекс, будут заблокированы до освобождения мьютекса. Если при освобождении
/// мьютекса происходит разблокировка ожидающей его задачи, то владение мьютексом переходит
/// к разблокированной задаче. 
/// @copyright AstroSoft Ltd, 2016
 
#pragma once

#include "macs_scheduler.hpp"

namespace macs {

/// @brief Класс для представления мьютекса.
/// @details Реализует мьютексы с поддержкой механизма наследования приоритетов, когда
/// низкоприоритетная задача на период владения мьютексом получает значение приоритета
/// равное максимальному приоритету среди ожидающих освобождения мьютекса задач. После
/// освобождения мьютекса низкоприоритетной задаче возвращается изначальное значение приоритета.
class Mutex : public SyncOwnedObject
{
private:
	uint32_t m_lock_cnt; // Количество блокировок: 0 или 1 для нерекурсивного мьютекса, 0 или 1+ для рекурсивного (0 означает свободный мьютекс)
	bool m_recursive;	// Признак рекурсивного мьютекса
public:
	/// @brief Конструктор мьютекса
	/// @details Создаёт объект мьютекса указанного типа.
	/// @param recursive - тип создаваемого мьютекса (true - рекурсивный, false - нерекурсивный).
	Mutex(bool recursive = false) : m_lock_cnt(0), m_recursive(recursive) { }

	~Mutex();

	/// @brief Проверка на рекурсивный мьютекс.
	/// @details Возвращает истину для рекурсивного мьютекса.
	/// @return Истина, если мьютекс является рекурсивным, ложь в противном случае.
	bool IsRecursive() const { return m_recursive; }

	/// @brief Захватить мьютекс
	/// @details Пытается захватить мьютекс в течение заданного времени. В случае нерекурсивного
	/// мьютекса одна и та же задача не должна пытаться повторно захватить мьютекс, который ею
	/// уже захвачен (ошибка ResultErrorInvalidState).
	/// @param timeout_ms - таймаут в миллисекундах. Для бесконечного ожидания - macs::INFINITE_TIMEOUT.
	/// @return [Результат операции](@ref macs::Result)
	virtual Result Lock(uint32_t timeout_ms = INFINITE_TIMEOUT);

	/// @brief Отпустить мьютекс
	/// @details Освобождает ранее захваченный мьютекс. Попытка освободить мьютекс,
	/// захваченный другой задачей, или не захваченный никем, считается ошибкой (ResultErrorInvalidState).
	/// @return [Результат операции](@ref macs::Result)
	virtual Result Unlock();

	/// @brief Получить состояние мьютекса
	/// @details Считывает состояние мьютекса в данный момент, не блокируя вызвавшую метод задачу.
	/// @return Истина, если мьютекс захвачен, ложь в противном случае.	
	bool IsLocked() const { return !! m_owner; }

	static Result Lock_Priv(Mutex * mutex, uint32_t timeout_ms); // Только для вызова ядром
	static Result Unlock_Priv(Mutex * mutex);                    // Только для вызова ядром

private:
	CLS_COPY(Mutex)

	virtual Result BlockCurTask(uint32_t timeout_ms);
	virtual Result UnblockTask();
	virtual void OnUnblockTask(Task *, Task::UnblockReason);
	virtual void OnDeleteTask(Task *);
#if MACS_MUTEX_PRIORITY_INVERSION
	inline Task::Priority RemoveFromOwner()
#else
	inline void RemoveFromOwner()
#endif
	{
		m_owner->RemoveOwnedSync(this);
#if MACS_MUTEX_PRIORITY_INVERSION
		Task::Priority inh_prior = m_owner_original_priority;
		SyncOwnedObject * pobj = m_owner->m_owned_obj_list;
		while ( pobj ) {
			if ( pobj->m_blocked_task_list ) {
				Task::Priority prior = pobj->m_blocked_task_list->GetPriority();
				if ( prior > inh_prior )
					inh_prior = prior;
			}
			pobj = OwnedSyncObjList::Next(pobj);
		}
		return inh_prior;
#endif
	}

	Result UnlockInternal();
	void UpdateOwnerPriority();
};

/// @brief Автоматический мьютекс (страж)
/// @details Динамическая переменная данного класса захватывает мьютекс в точке 
/// своего объявления и освобождает его когда кончается ее область действия.
class MutexGuard
{
public:
	/// @brief Конструктор автоматического мьютекса
	/// @param mutex - мьютекс, который будет захвачен в конструкторе и освобожден в деструкторе
	/// @param only_unlock - если истина, не захватывать мьютекс в конструкторе
	explicit MutexGuard(Mutex & mutex, bool only_unlock = false) :
		m_mutex(mutex)
	{
		if ( ! only_unlock )
			m_mutex.Lock();
	}

	/// @brief Деструктор автоматического мьютекса
	/// @details Освобождает указанный при создании мьютекс
	~MutexGuard()
	{
		m_mutex.Unlock();
	}

private:
	CLS_COPY(MutexGuard)

private:
	Mutex & m_mutex;
};

}	// namespace macs
