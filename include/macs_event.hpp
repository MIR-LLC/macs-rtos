/// @file macs_event.hpp
/// @brief События.
/// @details События могут быть использованы в тех случаях, когда необходимо заблокировать
/// задачу до тех пор, пока не возникнет какое-либо условие для её разблокировки. 
/// @copyright AstroSoft Ltd, 2016

#pragma once

#include "macs_scheduler.hpp"

namespace macs {

/// @brief Класс для представления событий.
/// @details Реализует событие без поддержки сигнального состояния: если метод Wait вызван
/// после вызова метода Raise, то ожидающий процесс будет заблокирован.
class Event : public SyncObject
{
public:
	/// @brief Конструктор события
	/// @details Создает объект события.
	/// @param broadcast Если истина, разблокируются все задачи, ожидавшие данного события,
	/// в противном случае только одна.
	Event(bool broadcast = true);

	~Event();

	/// @brief Получить тип события
	/// @details Возвращает тип события, указанный при создании объекта.
	/// @return true - для широковещательного события, false - в противном случае.
 	bool GetBroadcast() { return m_broadcast; };

	/// @brief Ждать наступления события
	/// @details Ожидает наступления данного события.
	/// @param timeout_ms таймаут в миллисекундах. Для бесконечного ожидания - macs::INFINITE_TIMEOUT.
	/// @return [Результат операции](@ref macs::Result)
	Result Wait(uint32_t timeout_ms = INFINITE_TIMEOUT);

	
	/// @brief Сигнализировать о наступлении события
	/// @details Дает сигнал на разблокировку задачи или всех задач, ожидающих данного события.
	/// @return [Результат операции](@ref macs::Result)
	Result Raise();

	static Result Wait_Priv(Event* event, uint32_t timeout_ms); // Только для вызова ядром
	static Result Raise_Priv(Event* event);                     // Только для вызова ядром

private:
	CLS_COPY(Event)

private:
	bool m_broadcast;
};

}	// namespace macs

