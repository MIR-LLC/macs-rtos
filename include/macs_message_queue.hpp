/// @file macs_message_queue.hpp
/// @brief Очереди сообщений.
/// @details Очередь сообщений имеет фиксированный размер.
/// Если очередь пуста, то задача, пытающаяся получить сообщение из очереди будет заблокирована
/// до появления в очереди нового сообщения. Если очередь заполнена, то задача, пытающаяся поместить
/// сообщение в очередь будет заблокирована до извлечения сообщения из очереди. 
/// @copyright AstroSoft Ltd, 2016
 
#pragma once

#include <stddef.h>
 
#include "macs_common.hpp"
#include "macs_critical_section.hpp"
#include "macs_semaphore.hpp"

namespace macs {

/// @brief Интерфейс очередей сообщений.
/// @details Шаблон класса представляет собой абстракцию для работы с очередями сообщений.
/// Параметром шаблона является тип сообщения (например, скалярный тип, структура или класс).
template <typename T>
	class MessageQueueInterface
{
public:
	
	/// @brief Поставить сообщение в конец очереди
	/// @details Помещает сообщение в конец очереди. Если очередь переполнена,
	/// задача будет заблокирована до появления свободного места, либо до истечения таймаута.
	/// @param message - ссылка на сообщение
	/// @param timeout_ms - таймаут в миллисекундах. Для бесконечного ожидания - macs::INFINITE_TIMEOUT.
	/// @return [Результат операции](@ref macs::Result)
	virtual Result Push(const T & message, uint32_t timeout_ms = INFINITE_TIMEOUT) = 0;

	/// @brief Поставить сообщение в начало очереди
	/// @details Помещает сообщение в начало очереди. Если очередь переполнена,
	/// задача будет заблокирована до появления свободного места, либо до истечения таймаута.
	/// @param message - ссылка на сообщение
	/// @param timeout_ms - таймаут в миллисекундах. Для бесконечного ожидания - macs::INFINITE_TIMEOUT.
	/// @return [Результат операции](@ref macs::Result)
	virtual Result PushFront(const T & message, uint32_t timeout_ms = INFINITE_TIMEOUT) = 0;

	/// @brief Извлечь сообщение из очереди
	/// @details Извлекает сообщение из начала очереди. Если очередь пуста,
	/// задача будет заблокирована до появления сообщения, либо до истечения таймаута.
	/// @param message - переменная, в которую будет помещена ссылка на сообщение
	/// @param timeout_ms - таймаут в миллисекундах. Для бесконечного ожидания - macs::INFINITE_TIMEOUT.
	/// @return [Результат операции](@ref macs::Result)
	virtual Result Pop(T & message, uint32_t timeout_ms = INFINITE_TIMEOUT) = 0;

	/// @brief Получить ссылку на первое сообщение очереди
	/// @details Получает ссылку на сообщение из начала очереди, не удаляя его из очереди. Если очередь пуста,
	/// задача будет заблокирована до появления сообщения, либо до истечения таймаута.
	/// @param message - переменная, в которую будет помещено сообщение
	/// @param timeout_ms - таймаут в миллисекундах. Для бесконечного ожидания - macs::INFINITE_TIMEOUT.
	/// @return [Результат операции](@ref macs::Result)
	virtual Result Peek(T & message, uint32_t timeout_ms = INFINITE_TIMEOUT) = 0;

	/// @brief Получить количество сообщений в очереди
	/// @return Текущее количество сообщений в очереди
	virtual size_t Count() const = 0;

	/// @brief Получить максимальную длину очереди
	/// @return Максимально возможное количество сообщений в очереди
	virtual size_t GetMaxSize() const = 0;

};	// class MessageQueueInterface

/// @brief Шаблон класса для представления очереди сообщений
/// @details Класс реализует функционал обмена сообщениями между задачами приложения (на одном процессоре).
/// Параметром шаблона является тип сообщения (например, скалярный тип, структура или класс).
template <typename T>
	class MessageQueue : public MessageQueueInterface<T>
{
public:
	/// @brief Конструктор очереди сообщений
	/// @param max_size - максимально допустимое количество элементов в очереди
	/// @param mem - указатель на внешнюю память для хранения элементов (должна вмещать max_size + 1 элемент). 
	MessageQueue(size_t max_size, T * mem = nullptr);
	~MessageQueue();

	/// @brief Поставить сообщение в конец очереди
	/// @details Помещает сообщение в конец очереди. Если очередь переполнена,
	/// задача будет заблокирована до появления свободного места, либо до истечения таймаута.
	/// @param message - ссылка на сообщение
	/// @param timeout_ms - таймаут в миллисекундах. Для бесконечного ожидания - macs::INFINITE_TIMEOUT.
	/// @return [Результат операции](@ref macs::Result)
	virtual Result Push(const T & message, uint32_t timeout_ms = INFINITE_TIMEOUT);

	/// @brief Поставить сообщение в начало очереди
	/// @details Помещает сообщение в начало очереди. Если очередь переполнена,
	/// задача будет заблокирована до появления свободного места, либо до истечения таймаута.
	/// @param message - ссылка на сообщение
	/// @param timeout_ms - таймаут в миллисекундах. Для бесконечного ожидания - macs::INFINITE_TIMEOUT.
	/// @return [Результат операции](@ref macs::Result)
	virtual Result PushFront(const T & message, uint32_t timeout_ms = INFINITE_TIMEOUT);

	/// @brief Извлечь сообщение из очереди
	/// @details Извлекает сообщение из начала очереди. Если очередь пуста,
	/// задача будет заблокирована до появления сообщения, либо до истечения таймаута.
	/// @param message - переменная, в которую будет помещена ссылка на сообщение
	/// @param timeout_ms - таймаут в миллисекундах. Для бесконечного ожидания - macs::INFINITE_TIMEOUT.
	/// @return [Результат операции](@ref macs::Result)
	virtual Result Pop(T & message, uint32_t timeout_ms = INFINITE_TIMEOUT);

	/// @brief Получить ссылку на первое сообщение очереди
	/// @details Получает ссылку на сообщение из начала очереди, не удаляя его из очереди. Если очередь пуста,
	/// задача будет заблокирована до появления сообщения, либо до истечения таймаута.
	/// @param message - переменная, в которую будет помещено сообщение
	/// @param timeout_ms - таймаут в миллисекундах. Для бесконечного ожидания - macs::INFINITE_TIMEOUT.
	/// @return [Результат операции](@ref macs::Result)
	virtual Result Peek(T & message, uint32_t timeout_ms = INFINITE_TIMEOUT);

	/// @brief Получить количество сообщений в очереди
	/// @return Текущее количество сообщений в очереди
	virtual size_t Count() const;

	/// @brief Получить максимальную длину очереди
	/// @return Максимально возможное количество сообщений в очереди
	virtual size_t GetMaxSize() const { return m_len - 1; }	
	 
private:
	CLS_COPY(MessageQueue)

	enum ACTION {
		QA_PUSH_FRONT,
		QA_PUSH_BACK, 
		QA_POP, 
		QA_PEEK
	};
	Result ProcessMessage(Semaphore & wait_sem, Semaphore & sig_sem, T & message, ACTION action, uint32_t timeout_ms);
	
private:
	const size_t m_len;
	Semaphore m_sem_read;
	Semaphore m_sem_write;
	
	bool m_is_alien_mem;
	T * m_memory;
	T * m_head_ptr, * m_tail_ptr;	// Любая операция может изменить значение только одного указателя.
};
	 
template <typename T>
	MessageQueue<T>::MessageQueue(size_t max_size, T * mem) :
		m_len(max_size + 1),	// Добавляем один граничный элемент, чтобы различать пустую и заполненную очередь.
		m_sem_read(0, max_size),
		m_sem_write(max_size, max_size)
{
	if ( ! mem ) {
		m_memory = new T[m_len];
		m_is_alien_mem = false;
	} else {
		m_memory = mem;
		m_is_alien_mem = true;
	}
	m_head_ptr = m_tail_ptr = m_memory;
}

template <typename T>
	MessageQueue<T>::~MessageQueue()
{
	if ( ! m_is_alien_mem )
		delete [] m_memory;
}

template <typename T>
	inline size_t MessageQueue<T>::Count() const 
{ 
	long diff = m_tail_ptr - m_head_ptr; 
	return diff >= 0 ? diff : (m_len + diff); 
}

template <typename T>
	inline Result MessageQueue<T>::Push(const T & message, uint32_t timeout_ms)
{
	return ProcessMessage(m_sem_write, m_sem_read, const_cast<T &>(message), QA_PUSH_BACK, timeout_ms);
}
	 
template <typename T>
	inline Result MessageQueue<T>::PushFront(const T & message, uint32_t timeout_ms)
{
	return ProcessMessage(m_sem_write, m_sem_read, const_cast<T &>(message), QA_PUSH_FRONT, timeout_ms);
}

template <typename T>
	inline Result MessageQueue<T>::Pop(T & message, uint32_t timeout_ms)
{
	return ProcessMessage(m_sem_read, m_sem_write, message, QA_POP, timeout_ms);
}

template <typename T>
	inline Result MessageQueue<T>::Peek(T & message, uint32_t timeout_ms)
{
	return ProcessMessage(m_sem_read, m_sem_read, message, QA_PEEK, timeout_ms);
}

template <typename T>
	Result MessageQueue<T>::ProcessMessage(Semaphore & wait_sem, Semaphore & sig_sem, T & message, ACTION action, uint32_t timeout_ms)
{
	if ( ! Sch().IsInitialized() || ! Sch().IsStarted() ) 
		return ResultErrorInvalidState;
	
	if ( ! timeout_ms ) {
		if ( ! System::IsSysCallAllowed() ) 
			return ResultErrorSysCallNotAllowed;
	} else {		
		if ( System::IsInInterrupt() ) 
			return ResultErrorInterruptNotSupported;
	}

	Result retcode = wait_sem.Wait(System::IsInInterrupt() ? 0 : timeout_ms);

	if ( retcode != ResultOk ) 
		return retcode;
	
	{
		PauseSection _ps_;		
		switch ( action ) {		
		case QA_PUSH_FRONT :
			{
				_ASSERT(Count() < GetMaxSize());
				if ( m_head_ptr == m_memory )
					m_head_ptr = & m_memory[m_len];
				* -- m_head_ptr = message;
			}
			break;
		case QA_PUSH_BACK :
			{
				_ASSERT(Count() < GetMaxSize());
				* m_tail_ptr ++ = message;
				if ( m_tail_ptr - m_memory == m_len )
					m_tail_ptr = m_memory;
			}
			break;
		case QA_POP :
			{
				_ASSERT(Count() != 0);
				message = * m_head_ptr ++;
				if ( m_head_ptr - m_memory == m_len )
					m_head_ptr = m_memory;
			}
			break;
		case QA_PEEK :
			{
				_ASSERT(Count() != 0);
				message = * m_head_ptr;
			}
			break;
		}
	}		

	retcode = sig_sem.Signal();

	return retcode;
}

}  // namespace macs
