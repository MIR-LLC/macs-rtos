/// @file macs_semaphore.hpp
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

#pragma once

#include "macs_scheduler.hpp" 

namespace macs {
	 
/// @brief Класс для представления семафоров
class Semaphore : public SyncObject
{
public:
	/// @brief Конструктор семафора
	/// @details Создаёт объект семафора с указанными параметрами (по умолчанию создается бинарный семафор).
	/// Если значение параметра start_count больше max_count, то начальное значение счётчика
    /// семафора будет установлено в max_count.
	/// @param start_count - начальное значение счётчика семафора
	/// @param max_count - максимально возможное значение счётчика семафора
 	Semaphore(size_t start_count = 0, size_t max_count = 1);

	~Semaphore();

	/// @brief Получить текущее значение счётчика семафора
	/// @return Текущее значение счётчика семафора
	size_t GetCurrentCount() const { return m_count; };
	
	/// @brief Получить максимальное значение счётчика семафора
	/// @return Максимально допустимое значение счётчика, указанное при создании семафора
	size_t GetMaxCount() const { return m_max_count; };

	/// @brief Ждать открытия семафора
	/// @details Блокирует задачу до тех пор, пока значение счётчика семафора не станет отличным от 
	/// нуля. После чего уменьшает значение счётчика и разблокирует задачу.
	/// @param timeout_ms - таймаут в миллисекундах. Для бесконечного ожидания - macs::INFINITE_TIMEOUT.
	/// @return [Результат операции](@ref macs::Result)
	Result Wait(uint32_t timeout_ms = INFINITE_TIMEOUT);

	/// @brief Сигнализировать об открытии семафора
	/// @details Увеличивает значение счётчика семафора на 1. 
	/// При этом возможна разблокировка одной из задач, выполнявших для данного семафора метод Wait().
	/// @return [Результат операции](@ref macs::Result)
	Result Signal();

	static Result Wait_Priv(Semaphore * semaphore, uint32_t timeout_ms); // Только для вызова ядром
	static Result Signal_Priv(Semaphore * semaphore);                    // Только для вызова ядром
	 
private:
	CLS_COPY(Semaphore)

	bool TryDecrement() { return m_count ? (-- m_count, true) : false; }

private:
	size_t m_count;
	size_t m_max_count;
};

/// Класс для представления бинарного семафора
class BinarySemaphore : public Semaphore
{
public:
	/// @brief Конструктор бинарного семафора
	/// @details Создает семафор с максимальным значением счетчика 1.
	/// @param is_empty - если истина, начальное значение семафора 0, иначе 1
	explicit BinarySemaphore(bool is_empty = true) : 
		Semaphore(is_empty ? 0 : 1, 1) 
	{}
};

}	// namespace macs
