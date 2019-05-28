/** @copyright AstroSoft Ltd */

#include "macs_tunes.h"

#if MACS_USE_TIMERS

#include "nullptr.h"
#include "macs_timer.hpp"

// параметры делителя частоты шины для разных режимов
const uint32_t Timer::TIMER_MS_PRESCALER = 21000 - 1;
const uint32_t Timer::TIMER_US_PRESCALER = 840 - 1;

// служебные структуры для таймеров
Timer::TimerData Timer::s_timers[NUMBER_OF_TIMERS];

// таблицы соответствия GPIO пинов PWM таймерам
const Timer::PWMTimerGPIOx Timer::s_port_A[NUMBER_OF_PORTS][NUMBER_OF_PORT_TIMERS] = {
/* PA0 */{ {TIM2, TIM_CHANNEL_1}, {TIM5, TIM_CHANNEL_1}},
/* PA1 */{ {TIM2, TIM_CHANNEL_2}, {TIM5, TIM_CHANNEL_2}},
/* PA2 */{ {TIM2, TIM_CHANNEL_3}, {TIM5, TIM_CHANNEL_3}},
/* PA3 */{ {TIM2, TIM_CHANNEL_4}, {TIM9, TIM_CHANNEL_2}},
/* PA4 */{ {0, 0}, {0, 0}},
/* PA5 */{ {TIM2, TIM_CHANNEL_1}, {0, 0}},
/* PA6 */{ {TIM3, TIM_CHANNEL_1}, {TIM13, TIM_CHANNEL_1}},
/* PA7 */{ {TIM14, TIM_CHANNEL_1}, {TIM3, TIM_CHANNEL_2}},
/* PA8 */{ {TIM1, TIM_CHANNEL_1}, {0, 0}},
/* PA9 */{ {TIM1, TIM_CHANNEL_2}, {0, 0}},
/* PA10 */{ {TIM1, TIM_CHANNEL_3}, {0, 0}},
/* PA11 */{ {TIM1, TIM_CHANNEL_4}, {0, 0}},
/* PA12 */{ {0, 0}, {0, 0}},
/* PA13 */{ {0, 0}, {0, 0}},
/* PA14 */{ {0, 0}, {0, 0}},
/* PA15 */{ {TIM2, TIM_CHANNEL_1}, {0, 0}}};

const Timer::PWMTimerGPIOx Timer::s_port_B[NUMBER_OF_PORTS][NUMBER_OF_PORT_TIMERS] = {
/* PB0 */{ {TIM3, TIM_CHANNEL_3}, {0, 0}},
/* PB1 */{ {0, 0}, {0, 0}},
/* PB2 */{ {0, 0}, {0, 0}},
/* PB3 */{ {TIM2, TIM_CHANNEL_2}, {0, 0}},
/* PB4 */{ {TIM3, TIM_CHANNEL_1}, {0, 0}},
/* PB5 */{ {TIM3, TIM_CHANNEL_2}, {0, 0}},
/* PB6 */{ {TIM4, TIM_CHANNEL_1}, {0, 0}},
/* PB7 */{ {TIM4, TIM_CHANNEL_2}, {0, 0}},
/* PB8 */{ {TIM4, TIM_CHANNEL_3}, {TIM10, TIM_CHANNEL_1}},
/* PB9 */{ {TIM4, TIM_CHANNEL_4}, {TIM11, TIM_CHANNEL_1}},
/* PB10 */{ {TIM2, TIM_CHANNEL_3}, {0, 0}},
/* PB11 */{ {TIM2, TIM_CHANNEL_4}, {0, 0}},
/* PB12 */{ {0, 0}, {0, 0}},
/* PB13 */{ {0, 0}, {0, 0}},
/* PB14 */{ {TIM12, TIM_CHANNEL_1}, {0, 0}},
/* PB15 */{ {TIM12, TIM_CHANNEL_2}, {0, 0}}};

const Timer::PWMTimerGPIOx Timer::s_port_C[NUMBER_OF_PORTS][NUMBER_OF_PORT_TIMERS] = {
/* PC0 */{ {0, 0}, {0, 0}},
/* PC1 */{ {0, 0}, {0, 0}},
/* PC2 */{ {0, 0}, {0, 0}},
/* PC3 */{ {0, 0}, {0, 0}},
/* PC4 */{ {0, 0}, {0, 0}},
/* PC5 */{ {0, 0}, {0, 0}},
/* PC6 */{ {TIM8, TIM_CHANNEL_1}, {TIM3, TIM_CHANNEL_1}},
/* PC7 */{ {TIM8, TIM_CHANNEL_2}, {TIM3, TIM_CHANNEL_2}},
/* PC8 */{ {TIM8, TIM_CHANNEL_3}, {TIM3, TIM_CHANNEL_3}},
/* PC9 */{ {TIM8, TIM_CHANNEL_4}, {TIM3, TIM_CHANNEL_4}},
/* PC10 */{ {0, 0}, {0, 0}},
/* PC11 */{ {0, 0}, {0, 0}},
/* PC12 */{ {0, 0}, {0, 0}},
/* PC13 */{ {0, 0}, {0, 0}},
/* PC14 */{ {0, 0}, {0, 0}},
/* PC15 */{ {0, 0}, {0, 0}}};

const Timer::PWMTimerGPIOx Timer::s_port_D[NUMBER_OF_PORTS][NUMBER_OF_PORT_TIMERS] = {
/* PD0 */{ {0, 0}, {0, 0}},
/* PD1 */{ {0, 0}, {0, 0}},
/* PD2 */{ {0, 0}, {0, 0}},
/* PD3 */{ {0, 0}, {0, 0}},
/* PD4 */{ {0, 0}, {0, 0}},
/* PD5 */{ {0, 0}, {0, 0}},
/* PD6 */{ {0, 0}, {0, 0}},
/* PD7 */{ {0, 0}, {0, 0}},
/* PD8 */{ {0, 0}, {0, 0}},
/* PD9 */{ {0, 0}, {0, 0}},
/* PD10 */{ {0, 0}, {0, 0}},
/* PD11 */{ {0, 0}, {0, 0}},
/* PD12 */{ {TIM4, TIM_CHANNEL_1}, {0, 0}},
/* PD13 */{ {TIM4, TIM_CHANNEL_2}, {0, 0}},
/* PD14 */{ {TIM4, TIM_CHANNEL_3}, {0, 0}},
/* PD15 */{ {TIM4, TIM_CHANNEL_4}, {0, 0}}};

const Timer::PWMTimerGPIOx Timer::s_port_E[NUMBER_OF_PORTS][NUMBER_OF_PORT_TIMERS] = {
/* PE0 */{ {0, 0}, {0, 0}},
/* PE1 */{ {0, 0}, {0, 0}},
/* PE2 */{ {0, 0}, {0, 0}},
/* PE3 */{ {0, 0}, {0, 0}},
/* PE4 */{ {0, 0}, {0, 0}},
/* PE5 */{ {TIM9, TIM_CHANNEL_1}, {0, 0}},
/* PE6 */{ {TIM9, TIM_CHANNEL_2}, {0, 0}},
/* PE7 */{ {0, 0}, {0, 0}},
/* PE8 */{ {0, 0}, {0, 0}},
/* PE9 */{ {TIM1, TIM_CHANNEL_1}, {0, 0}},
/* PE10 */{ {TIM1, TIM_CHANNEL_2}, {0, 0}},
/* PE11 */{ {0, 0}, {0, 0}},
/* PE12 */{ {0, 0}, {0, 0}},
/* PE13 */{ {TIM1, TIM_CHANNEL_3}, {0, 0}},
/* PE14 */{ {TIM1, TIM_CHANNEL_4}, {0, 0}},
/* PE15 */{ {0, 0}, {0, 0}}};

const Timer::PWMTimerGPIOx Timer::s_port_F[NUMBER_OF_PORTS][NUMBER_OF_PORT_TIMERS] = {
/* PF0 */{ {0, 0}, {0, 0}},
/* PF1 */{ {0, 0}, {0, 0}},
/* PF2 */{ {0, 0}, {0, 0}},
/* PF3 */{ {0, 0}, {0, 0}},
/* PF4 */{ {0, 0}, {0, 0}},
/* PF5 */{ {0, 0}, {0, 0}},
/* PF6 */{ {TIM10, TIM_CHANNEL_1}, {0, 0}},
/* PF7 */{ {TIM11, TIM_CHANNEL_1}, {0, 0}},
/* PF8 */{ {TIM13, TIM_CHANNEL_1}, {0, 0}},
/* PF9 */{ {TIM14, TIM_CHANNEL_1}, {0, 0}},
/* PF10 */{ {0, 0}, {0, 0}},
/* PF11 */{ {0, 0}, {0, 0}},
/* PF12 */{ {0, 0}, {0, 0}},
/* PF13 */{ {0, 0}, {0, 0}},
/* PF14 */{ {0, 0}, {0, 0}},
/* PF15 */{ {0, 0}, {0, 0}}};

Timer::Timer() :
	m_timer(nullptr), m_channel(0)
{
}

Timer::~Timer()
{
	DeInitialize();
}

// период задаётся в зависимости от выбранного режима:
// в микросекундах (минимум 10, кратно 5, округление вниз)
// или в миллисекундах (минимум 1, кратно единице)

// инициализация таймера в обычном режиме (генерирует прерывания)
Result Timer::Initialize(unsigned int period, MeasureMode mode, TimerAction* action)
{
	for (int i = 0; i < NUMBER_OF_TIMERS; ++i) {
		if (s_timers[i].m_mode == Inactive) {
			m_timer = &s_timers[i];
			m_timer->m_handle.Instance = GetTimerHandle(i);
			break;
		}
	}

	if (m_timer == 0)
		return ResultErrorInvalidState;

	m_timer->m_mode = Basic;
	m_timer->m_action = action;

	if (mode == Microseconds) {
		m_timer->m_handle.Init.Prescaler = TIMER_US_PRESCALER;
		m_timer->m_handle.Init.Period = UsToPeriod(period);
	} else {
		m_timer->m_handle.Init.Prescaler = TIMER_MS_PRESCALER;
		m_timer->m_handle.Init.Period = MsToPeriod(period);
	}

	m_timer->m_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
	m_timer->m_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

	if (HAL_TIM_Base_Init(&m_timer->m_handle) != HAL_OK)
		return ResultErrorInvalidState;

	return ResultOk;
}

// инициализация таймера в режиме ШИМ (генерирует ШИМ-сигнал)
Result Timer::Initialize(unsigned int period, unsigned int pulse_width, MeasureMode mode, GPIO_TypeDef* gpio_x, uint32_t pin)
{
	// поиск доступного таймера для соответствующего порта
	TIM_TypeDef* htim;
	PWMTimerGPIOx* portx;

	if (gpio_x == GPIOA)
		portx = (PWMTimerGPIOx*)&s_port_A[0][0];
	else if (gpio_x == GPIOB)
		portx = (PWMTimerGPIOx*)&s_port_B[0][0];
	else if (gpio_x == GPIOC)
		portx = (PWMTimerGPIOx*)&s_port_C[0][0];
	else if (gpio_x == GPIOD)
		portx = (PWMTimerGPIOx*)&s_port_D[0][0];
	else if (gpio_x == GPIOE)
		portx = (PWMTimerGPIOx*)&s_port_E[0][0];
	else if (gpio_x == GPIOF)
		portx = (PWMTimerGPIOx*)&s_port_F[0][0];
	else
		return ResultErrorInvalidState;

	int pinInd = 31 - __CLZ(pin);
	PWMTimerGPIOx* timer1 = portx + pinInd * 2 + 0;
	PWMTimerGPIOx* timer2 = portx + pinInd * 2 + 1;

	if (timer1->m_hardware_timer != 0) {
		htim = timer1->m_hardware_timer;
		m_channel = timer1->m_channel;
	} else if (timer2->m_hardware_timer != 0) {
		htim = timer2->m_hardware_timer;
		m_channel = timer2->m_channel;
	} else
		return ResultErrorNotSupported;

	m_timer = &s_timers[GetTimerIndex(htim)];

	m_timer->m_handle.Instance = htim;
	m_timer->m_mode = PWM;
	m_timer->m_gpio_x = gpio_x;

	// настройка таймера
	if (mode == Microseconds) {
		m_timer->m_handle.Init.Prescaler = TIMER_US_PRESCALER;
		m_timer->m_handle.Init.Period = UsToPeriod(period);
	} else {
		m_timer->m_handle.Init.Prescaler = TIMER_MS_PRESCALER;
		m_timer->m_handle.Init.Period = MsToPeriod(period);
	}

	m_timer->m_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
	m_timer->m_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

	if (HAL_TIM_PWM_Init(&m_timer->m_handle) != HAL_OK)
		return ResultErrorInvalidState;

	// настройка GPIO порта
	GPIO_InitTypeDef gpio;
	gpio.Pin = pin;
	gpio.Mode = GPIO_MODE_AF_PP;
	gpio.Pull = GPIO_PULLDOWN;
	gpio.Speed = GPIO_SPEED_FAST;

	if (htim == TIM1)
		gpio.Alternate = GPIO_AF1_TIM1;
	else if (htim == TIM2)
		gpio.Alternate = GPIO_AF1_TIM2;
	else if (htim == TIM3)
		gpio.Alternate = GPIO_AF2_TIM3;
	else if (htim == TIM4)
		gpio.Alternate = GPIO_AF2_TIM4;
	else if (htim == TIM5)
		gpio.Alternate = GPIO_AF2_TIM5;
	else if (htim == TIM8)
		gpio.Alternate = GPIO_AF3_TIM8;
	else if (htim == TIM9)
		gpio.Alternate = GPIO_AF3_TIM9;
	else if (htim == TIM10)
		gpio.Alternate = GPIO_AF3_TIM10;
	else if (htim == TIM11)
		gpio.Alternate = GPIO_AF3_TIM11;
	else if (htim == TIM12)
		gpio.Alternate = GPIO_AF9_TIM12;
	else if (htim == TIM13)
		gpio.Alternate = GPIO_AF9_TIM13;
	else if (htim == TIM14)
		gpio.Alternate = GPIO_AF9_TIM14;

	HAL_GPIO_Init(gpio_x, &gpio);

	// настройка PWM
	TIM_OC_InitTypeDef timerOC;
	timerOC.Pulse = mode == Microseconds ? UsToPeriod(pulse_width) : MsToPeriod(pulse_width);
	timerOC.OCMode = TIM_OCMODE_PWM1;
	timerOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	timerOC.OCFastMode = TIM_OCFAST_DISABLE;

	if (HAL_TIM_PWM_ConfigChannel(&m_timer->m_handle, &timerOC, m_channel) != HAL_OK)
		return ResultErrorInvalidState;

	return ResultOk;
}

Result Timer::DeInitialize()
{
	HAL_StatusTypeDef status = (HAL_StatusTypeDef)0xFF;

	if (m_timer->m_mode == Basic)
		status = HAL_TIM_Base_DeInit(&m_timer->m_handle);
	else if (m_timer->m_mode == PWM)
		status = HAL_TIM_PWM_DeInit(&m_timer->m_handle);

	m_timer->m_mode = Inactive;

	return status == HAL_OK ? ResultOk : ResultErrorInvalidState;
}

Result Timer::Start(bool immediate_tick)
{
	HAL_StatusTypeDef status = (HAL_StatusTypeDef)0xFF;

	if (!immediate_tick)
		__HAL_TIM_CLEAR_IT(&m_timer->m_handle, TIM_IT_UPDATE);

	if (m_timer->m_mode == Basic)
		status = HAL_TIM_Base_Start_IT(&m_timer->m_handle);
	else if (m_timer->m_mode == PWM)
		status = HAL_TIM_PWM_Start(&m_timer->m_handle, m_channel);

	return status == HAL_OK ? ResultOk : ResultErrorInvalidState;
}

Result Timer::Stop()
{
	HAL_StatusTypeDef status = (HAL_StatusTypeDef)0xFF;

	if (m_timer->m_mode == Basic)
		status = HAL_TIM_Base_Stop_IT(&m_timer->m_handle);
	else if (m_timer->m_mode == PWM)
		status = HAL_TIM_PWM_Stop(&m_timer->m_handle, m_channel);

	return status == HAL_OK ? ResultOk : ResultErrorInvalidState;
}

Result Timer::SetPulseWidth(unsigned int pulse_width, MeasureMode mode)
{
	if (m_timer->m_mode != PWM)
		return ResultErrorInvalidState;

	__HAL_TIM_SET_COMPARE(&m_timer->m_handle, m_channel,
		mode == Microseconds ? UsToPeriod(pulse_width) : MsToPeriod(pulse_width));

	return ResultOk;
}

int Timer::GetTimerIndex(TIM_TypeDef* hardware_timer)
{
	if (hardware_timer == TIM1)
		return 0;
	else if (hardware_timer == TIM2)
		return 1;
	else if (hardware_timer == TIM3)
		return 2;
	else if (hardware_timer == TIM4)
		return 3;
	else if (hardware_timer == TIM5)
		return 4;
	else if (hardware_timer == TIM6)
		return 5;
	else if (hardware_timer == TIM7)
		return 6;
	else if (hardware_timer == TIM8)
		return 7;
	else if (hardware_timer == TIM9)
		return 8;
	else if (hardware_timer == TIM10)
		return 9;
	else if (hardware_timer == TIM11)
		return 10;
	else if (hardware_timer == TIM12)
		return 11;
	else if (hardware_timer == TIM13)
		return 12;
	else if (hardware_timer == TIM14)
		return 13;
	else
		return -1;
}

TIM_TypeDef* Timer::GetTimerHandle(int index)
{
	switch (index) {
	case 0:
		return TIM1;
	case 1:
		return TIM2;
	case 2:
		return TIM3;
	case 3:
		return TIM4;
	case 4:
		return TIM5;
	case 5:
		return TIM6;
	case 6:
		return TIM7;
	case 7:
		return TIM8;
	case 8:
		return TIM9;
	case 9:
		return TIM10;
	case 10:
		return TIM11;
	case 11:
		return TIM12;
	case 12:
		return TIM13;
	case 13:
		return TIM14;
	default:
		return 0;
	}
}

extern "C"
{
// обработчики прерываний таймеров
void TIM1_BRK_TIM9_IRQHandler()
{
	if (Timer::s_timers[8].m_mode == Timer::Basic)
		HAL_TIM_IRQHandler(&Timer::s_timers[8].m_handle);
}

void TIM1_UP_TIM10_IRQHandler()
{
	if (Timer::s_timers[0].m_mode == Timer::Basic)
		HAL_TIM_IRQHandler(&Timer::s_timers[0].m_handle);

	if (Timer::s_timers[9].m_mode == Timer::Basic)
		HAL_TIM_IRQHandler(&Timer::s_timers[9].m_handle);
}

void TIM1_TRG_COM_TIM11_IRQHandler()
{
	if (Timer::s_timers[10].m_mode == Timer::Basic)
		HAL_TIM_IRQHandler(&Timer::s_timers[10].m_handle);
}

void TIM2_IRQHandler()
{
	if (Timer::s_timers[1].m_mode == Timer::Basic)
		HAL_TIM_IRQHandler(&Timer::s_timers[1].m_handle);
}

void TIM3_IRQHandler()
{
	if (Timer::s_timers[2].m_mode == Timer::Basic)
		HAL_TIM_IRQHandler(&Timer::s_timers[2].m_handle);
}

void TIM4_IRQHandler()
{
	if (Timer::s_timers[3].m_mode == Timer::Basic)
		HAL_TIM_IRQHandler(&Timer::s_timers[3].m_handle);
}

void TIM5_IRQHandler()
{
	if (Timer::s_timers[4].m_mode == Timer::Basic)
		HAL_TIM_IRQHandler(&Timer::s_timers[4].m_handle);
}

void TIM6_DAC_IRQHandler()
{
	if (Timer::s_timers[5].m_mode == Timer::Basic)
		HAL_TIM_IRQHandler(&Timer::s_timers[5].m_handle);
}

void TIM7_IRQHandler()
{
	if (Timer::s_timers[6].m_mode == Timer::Basic)
		HAL_TIM_IRQHandler(&Timer::s_timers[6].m_handle);
}

void TIM8_BRK_TIM12_IRQHandler()
{
	if (Timer::s_timers[11].m_mode == Timer::Basic)
		HAL_TIM_IRQHandler(&Timer::s_timers[11].m_handle);
}

void TIM8_UP_TIM13_IRQHandler()
{
	if (Timer::s_timers[7].m_mode == Timer::Basic)
		HAL_TIM_IRQHandler(&Timer::s_timers[7].m_handle);

	if (Timer::s_timers[12].m_mode == Timer::Basic)
		HAL_TIM_IRQHandler(&Timer::s_timers[12].m_handle);
}

void TIM8_TRG_COM_TIM14_IRQHandler()
{
	if (Timer::s_timers[13].m_mode == Timer::Basic)
		HAL_TIM_IRQHandler(&Timer::s_timers[13].m_handle);
}

// служебные функции HAL для таймеров
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	int timerInd = Timer::GetTimerIndex(htim->Instance);

	if (Timer::s_timers[timerInd].m_mode == Timer::Basic)
		(*Timer::s_timers[timerInd].m_action)();
}

void EnableGPIOxClock(GPIO_TypeDef* GPIOx)
{
	if (GPIOx == GPIOA)
		__GPIOA_CLK_ENABLE()
		;
	else if (GPIOx == GPIOB)
		__GPIOB_CLK_ENABLE()
		;
	else if (GPIOx == GPIOC)
		__GPIOC_CLK_ENABLE()
		;
	else if (GPIOx == GPIOD)
		__GPIOD_CLK_ENABLE();
	else if (GPIOx == GPIOE)
		__GPIOE_CLK_ENABLE();
	else if (GPIOx == GPIOF)
		__GPIOF_CLK_ENABLE();
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM1) {
		NVIC_EnableIRQ (TIM1_UP_TIM10_IRQn);
		__TIM1_CLK_ENABLE()
		;
	} else if (htim->Instance == TIM2) {
		NVIC_EnableIRQ (TIM2_IRQn);
		__TIM2_CLK_ENABLE();
	} else if (htim->Instance == TIM3) {
		NVIC_EnableIRQ (TIM3_IRQn);
		__TIM3_CLK_ENABLE();
	} else if (htim->Instance == TIM4) {
		NVIC_EnableIRQ (TIM4_IRQn);
		__TIM4_CLK_ENABLE();
	} else if (htim->Instance == TIM5) {
		NVIC_EnableIRQ (TIM5_IRQn);
		__TIM5_CLK_ENABLE()
		;
	} else if (htim->Instance == TIM6) {
		NVIC_EnableIRQ (TIM6_DAC_IRQn);
		__TIM6_CLK_ENABLE();
	} else if (htim->Instance == TIM7) {
		NVIC_EnableIRQ (TIM7_IRQn);
		__TIM7_CLK_ENABLE();
	} else if (htim->Instance == TIM8) {
		NVIC_EnableIRQ (TIM8_UP_TIM13_IRQn);
		__TIM8_CLK_ENABLE();
	} else if (htim->Instance == TIM9) {
		NVIC_EnableIRQ (TIM1_BRK_TIM9_IRQn);
		__TIM9_CLK_ENABLE()
		;
	} else if (htim->Instance == TIM10) {
		NVIC_EnableIRQ (TIM1_UP_TIM10_IRQn);
		__TIM10_CLK_ENABLE();
	} else if (htim->Instance == TIM11) {
		NVIC_EnableIRQ (TIM1_TRG_COM_TIM11_IRQn);
		__TIM11_CLK_ENABLE()
		;
	} else if (htim->Instance == TIM12) {
		NVIC_EnableIRQ (TIM8_BRK_TIM12_IRQn);
		__TIM12_CLK_ENABLE();
	} else if (htim->Instance == TIM13) {
		NVIC_EnableIRQ (TIM8_UP_TIM13_IRQn);
		__TIM13_CLK_ENABLE();
	} else if (htim->Instance == TIM14) {
		NVIC_EnableIRQ (TIM8_TRG_COM_TIM14_IRQn);
		__TIM14_CLK_ENABLE();
	}
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM1) {
		NVIC_DisableIRQ (TIM1_UP_TIM10_IRQn);
		__TIM1_CLK_DISABLE();
	} else if (htim->Instance == TIM2) {
		NVIC_DisableIRQ (TIM2_IRQn);
		__TIM2_CLK_DISABLE();
	} else if (htim->Instance == TIM3) {
		NVIC_DisableIRQ (TIM3_IRQn);
		__TIM3_CLK_DISABLE();
	} else if (htim->Instance == TIM4) {
		NVIC_DisableIRQ (TIM4_IRQn);
		__TIM4_CLK_DISABLE();
	} else if (htim->Instance == TIM5) {
		NVIC_DisableIRQ (TIM5_IRQn);
		__TIM5_CLK_DISABLE();
	} else if (htim->Instance == TIM6) {
		NVIC_DisableIRQ (TIM6_DAC_IRQn);
		__TIM6_CLK_DISABLE();
	} else if (htim->Instance == TIM7) {
		NVIC_DisableIRQ (TIM7_IRQn);
		__TIM7_CLK_DISABLE();
	} else if (htim->Instance == TIM8) {
		NVIC_DisableIRQ (TIM8_UP_TIM13_IRQn);
		__TIM8_CLK_DISABLE();
	} else if (htim->Instance == TIM9) {
		NVIC_DisableIRQ (TIM1_BRK_TIM9_IRQn);
		__TIM9_CLK_DISABLE();
	} else if (htim->Instance == TIM10) {
		NVIC_DisableIRQ (TIM1_UP_TIM10_IRQn);
		__TIM10_CLK_DISABLE();
	} else if (htim->Instance == TIM11) {
		NVIC_DisableIRQ (TIM1_TRG_COM_TIM11_IRQn);
		__TIM11_CLK_DISABLE();
	} else if (htim->Instance == TIM12) {
		NVIC_DisableIRQ (TIM8_BRK_TIM12_IRQn);
		__TIM12_CLK_DISABLE();
	} else if (htim->Instance == TIM13) {
		NVIC_DisableIRQ (TIM8_UP_TIM13_IRQn);
		__TIM13_CLK_DISABLE();
	} else if (htim->Instance == TIM14) {
		NVIC_DisableIRQ (TIM8_TRG_COM_TIM14_IRQn);
		__TIM14_CLK_DISABLE();
	}
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM1) {
		EnableGPIOxClock(Timer::s_timers[0].m_gpio_x);
		__TIM1_CLK_ENABLE()
		;
	} else if (htim->Instance == TIM2) {
		EnableGPIOxClock(Timer::s_timers[1].m_gpio_x);
		__TIM2_CLK_ENABLE();
	} else if (htim->Instance == TIM3) {
		EnableGPIOxClock(Timer::s_timers[2].m_gpio_x);
		__TIM3_CLK_ENABLE();
	} else if (htim->Instance == TIM4) {
		EnableGPIOxClock(Timer::s_timers[3].m_gpio_x);
		__TIM4_CLK_ENABLE();
	} else if (htim->Instance == TIM5) {
		EnableGPIOxClock(Timer::s_timers[4].m_gpio_x);
		__TIM5_CLK_ENABLE()
		;
	} else if (htim->Instance == TIM8) {
		EnableGPIOxClock(Timer::s_timers[7].m_gpio_x);
		__TIM8_CLK_ENABLE();
	} else if (htim->Instance == TIM9) {
		EnableGPIOxClock(Timer::s_timers[8].m_gpio_x);
		__TIM9_CLK_ENABLE()
		;
	} else if (htim->Instance == TIM10) {
		EnableGPIOxClock(Timer::s_timers[9].m_gpio_x);
		__TIM10_CLK_ENABLE();
	} else if (htim->Instance == TIM11) {
		EnableGPIOxClock(Timer::s_timers[10].m_gpio_x);
		__TIM11_CLK_ENABLE()
		;
	} else if (htim->Instance == TIM12) {
		EnableGPIOxClock(Timer::s_timers[11].m_gpio_x);
		__TIM12_CLK_ENABLE();
	} else if (htim->Instance == TIM13) {
		EnableGPIOxClock(Timer::s_timers[12].m_gpio_x);
		__TIM13_CLK_ENABLE();
	}
}

void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM1)
		__TIM1_CLK_DISABLE();
	else if (htim->Instance == TIM2)
		__TIM2_CLK_DISABLE();
	else if (htim->Instance == TIM3)
		__TIM3_CLK_DISABLE();
	else if (htim->Instance == TIM4)
		__TIM4_CLK_DISABLE();
	else if (htim->Instance == TIM5)
		__TIM5_CLK_DISABLE();
	else if (htim->Instance == TIM6)
		__TIM6_CLK_DISABLE();
	else if (htim->Instance == TIM7)
		__TIM7_CLK_DISABLE();
	else if (htim->Instance == TIM8)
		__TIM8_CLK_DISABLE();
	else if (htim->Instance == TIM9)
		__TIM9_CLK_DISABLE();
	else if (htim->Instance == TIM10)
		__TIM10_CLK_DISABLE();
	else if (htim->Instance == TIM11)
		__TIM11_CLK_DISABLE();
	else if (htim->Instance == TIM12)
		__TIM12_CLK_DISABLE();
	else if (htim->Instance == TIM13)
		__TIM13_CLK_DISABLE();
}
}

#endif	// #if MACS_USE_TIMERS
