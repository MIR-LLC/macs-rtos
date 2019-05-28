/// @file macs_timer.hpp
/// @brief Таймеры.
/// @details Реализует функционал для доступа и управления таймерами.
/// @copyright AstroSoft Ltd, 2015-2016

#pragma once

#include "macs_common.hpp"

/// @brief Базовый класс для действий, вызываемых по сигналу от таймера.
class TimerAction
{
public:
	/// @brief Оператор, выполняющий действия по сигналу от таймера.
	virtual void operator()() = 0;
};

struct TimerData;

/// @brief Таймер.
class Timer
{
public:
	/// @brief Конструктор. Создает объект для управления таймером.
	Timer();

	/// @brief Деструктор.
	~Timer();

	/// @brief Величина тика таймера.
	enum MeasureMode
	{
		Microseconds, ///< Микросекунды, [мкс]
		Milliseconds ///< Миллисекунды, [мс]
	};

	/// @brief Инициализирует таймер.
	/// @param period Количество тиков между сигналами от таймера.
	/// @param mode Величина одного тика.
	/// @param action Действие, выполняемое по сигналу от таймера.
	/// @return [ResultOk](@ref macs::ResultOk) - если операция завершена успешно, код ошибки - в противном случае.
	Result Initialize(uint period, MeasureMode mode, TimerAction * action);

	/// @brief Освобождает все ресурсы, занятые таймером.
	/// @return [ResultOk](@ref macs::ResultOk) - если операция завершена успешно, код ошибки - в противном случае.
	Result DeInitialize();

	/// @brief Начинает отсчет тиков и посылку сигналов.
	/// @param immediate_tick true - генерирует сигнал сразу при запуске, false - первый сигнал придет только по истечении периода таймера.
	/// @return [ResultOk](@ref macs::ResultOk) - если операция завершена успешно, код ошибки - в противном случае.
	Result Start(bool immediate_tick = false);

	/// @brief Останавливает отсчет тиков и посылку сигналов.
	/// @return [ResultOk](@ref macs::ResultOk) - если операция завершена успешно, код ошибки - в противном случае.
	Result Stop();

	ulong GetTick();
	void SetTick(ulong);
	ulong TicksFreq();
	ulong TicksMaxVal();

private:
	TimerData * m_timer;
};
