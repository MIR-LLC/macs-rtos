/// @file macs_timer.hpp
/// @brief Таймеры.
/// @details Реализует функционал для доступа и управления таймерами.
/// @copyright AstroSoft Ltd, 2015-2016

#pragma once

#include "macs_common.hpp"
#include "stm32f4xx_hal.h"

/// @brief Базовый класс для действий, вызываемых по сигналу от таймера.
class TimerAction
{
public:
	/// @brief Оператор, выполняющий действия по сигналу от таймера.
	virtual void operator()() = 0;
};

extern "C"
{
void TIM1_BRK_TIM9_IRQHandler();
void TIM1_UP_TIM10_IRQHandler();
void TIM1_TRG_COM_TIM11_IRQHandler();
void TIM2_IRQHandler();
void TIM3_IRQHandler();
void TIM4_IRQHandler();
void TIM5_IRQHandler();
void TIM6_DAC_IRQHandler();
void TIM7_IRQHandler();
void TIM8_BRK_TIM12_IRQHandler();
void TIM8_UP_TIM13_IRQHandler();
void TIM8_TRG_COM_TIM14_IRQHandler();
}

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
	Result Initialize(unsigned int period, MeasureMode mode, TimerAction* action);

	/// @brief Инициализирует таймер.
	/// @param period Количество тиков между сигналами от таймера.
	/// @param pulse_width Ширина импульса в ШИМ режиме.
	/// @param gpoi_x Порт GPIO.
	/// @param pin Номер пина в порту GPIO.
	/// @return [ResultOk](@ref macs::ResultOk) - если операция завершена успешно, код ошибки - в противном случае.
	Result Initialize(unsigned int period, unsigned int pulse_width, MeasureMode mode, GPIO_TypeDef* gpio_x, uint32_t pin);

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

	/// @brief Устанавливает ширину импульса в ШИМ режиме.
	/// @param pulse_width Ширина импульса в ШИМ режиме.
	/// @param mode Величина одного тика.
	/// @return [ResultOk](@ref macs::ResultOk) - если операция завершена успешно, код ошибки - в противном случае.
	Result SetPulseWidth(unsigned int pulse_width, MeasureMode mode);

private:
	friend void TIM1_BRK_TIM9_IRQHandler();
	friend void TIM1_UP_TIM10_IRQHandler();
	friend void TIM1_TRG_COM_TIM11_IRQHandler();
	friend void TIM2_IRQHandler();
	friend void TIM3_IRQHandler();
	friend void TIM4_IRQHandler();
	friend void TIM5_IRQHandler();
	friend void TIM6_DAC_IRQHandler();
	friend void TIM7_IRQHandler();
	friend void TIM8_BRK_TIM12_IRQHandler();
	friend void TIM8_UP_TIM13_IRQHandler();
	friend void TIM8_TRG_COM_TIM14_IRQHandler();

	friend void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
	friend void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim);

	static const uint32_t TIMER_MS_PRESCALER;
	static const uint32_t TIMER_US_PRESCALER;

	static const size_t NUMBER_OF_TIMERS = 14;
	static const size_t NUMBER_OF_PORTS = 16;
	static const size_t NUMBER_OF_PORT_TIMERS = 2;

	static unsigned int UsToPeriod(unsigned int us)
	{
		return us > 10 ? (us - 11) / 5 + 1 : 1;
	}
	static unsigned int MsToPeriod(unsigned int ms)
	{
		return ms > 1 ? 4 * ms - 1 : 7;
	}

	static int GetTimerIndex(TIM_TypeDef* hardware_timer);
	static TIM_TypeDef* GetTimerHandle(int index);

	enum TimerMode
	{
		Inactive,
		Basic,
		PWM,
		NotSupported
	};

	struct TimerData
	{
		TimerMode m_mode;
		TIM_HandleTypeDef m_handle;
		TimerAction* m_action;
		GPIO_TypeDef* m_gpio_x;
	};

	TimerData* m_timer;
	uint32_t m_channel;

	static TimerData s_timers[NUMBER_OF_TIMERS];

	struct PWMTimerGPIOx
	{
		TIM_TypeDef* m_hardware_timer;
		uint32_t m_channel;
	};

	static const PWMTimerGPIOx s_port_A[NUMBER_OF_PORTS][NUMBER_OF_PORT_TIMERS];
	static const PWMTimerGPIOx s_port_B[NUMBER_OF_PORTS][NUMBER_OF_PORT_TIMERS];
	static const PWMTimerGPIOx s_port_C[NUMBER_OF_PORTS][NUMBER_OF_PORT_TIMERS];
	static const PWMTimerGPIOx s_port_D[NUMBER_OF_PORTS][NUMBER_OF_PORT_TIMERS];
	static const PWMTimerGPIOx s_port_E[NUMBER_OF_PORTS][NUMBER_OF_PORT_TIMERS];
	static const PWMTimerGPIOx s_port_F[NUMBER_OF_PORTS][NUMBER_OF_PORT_TIMERS];
};
