/** @copyright AstroSoft Ltd */

#include "macs_tunes.h"

#if MACS_USE_TIMERS

#include "macs_nullptr.h"
#include "macs_system.hpp"
#include "macs_timer.hpp"
 
extern "C" {
#include "MDR1986VE1T.h"
#include "MDR32F9Qx_rst_clk.h"
#include "MDR32F9Qx_timer.h"
}

enum TimerMode
{
	TM_INACTIVE,  // Таймер свободен
	TM_BASIC      // Таймер занят
};

struct TimerData
{
	TimerMode m_mode;
	IRQn_Type m_irq_n;
	MDR_TIMER_TypeDef * m_handle;
	uint m_period;
	TimerAction * m_action;
};

// служебные структуры для таймеров
static TimerData s_timers[] = {
	{TM_INACTIVE, TIMER1_IRQn, MDR_TIMER1, 0, nullptr},
	{TM_INACTIVE, TIMER2_IRQn, MDR_TIMER2, 0, nullptr},
	{TM_INACTIVE, TIMER3_IRQn, MDR_TIMER3, 0, nullptr},
	{TM_INACTIVE, TIMER4_IRQn, MDR_TIMER4, 0, nullptr}};

static const uint NUMBER_OF_TIMERS = countof(s_timers);

inline uint GetTimerIndex(MDR_TIMER_TypeDef * hardware_timer)
{
	_ASSERT(hardware_timer == MDR_TIMER1 || hardware_timer == MDR_TIMER2 || hardware_timer == MDR_TIMER3 || hardware_timer == MDR_TIMER4);
	return hardware_timer == s_timers[0].m_handle ? 0 : 1;
}
static int GetFreeTimerIndex()
{
	loop ( uint, index, NUMBER_OF_TIMERS )
		if ( s_timers[index].m_mode == TM_INACTIVE )
			return index;
	return -1;
}

////////////////////////////////////////////////////////////////

Timer::Timer() :
	m_timer(nullptr)
{}

Timer::~Timer()
{
	DeInitialize();
}

// инициализация таймера в обычном режиме (генерирует прерывания)
Result Timer::Initialize(uint period, MeasureMode mode, TimerAction * action)
{
	static bool s_is_first_time = true;  // TODO: оформить в стиле остальной периферии	 
	if ( s_is_first_time ) {
		RST_CLK_PCLKcmd((RST_CLK_PCLK_RST_CLK | RST_CLK_PCLK_TIMER1 
		                                      | RST_CLK_PCLK_TIMER2
		                                      | RST_CLK_PCLK_TIMER3
		                                      | RST_CLK_PCLK_TIMER4), ENABLE);		
		s_is_first_time = false;
	}

	DeInitialize();
	
	if ( ! m_timer ) {
		int index = GetFreeTimerIndex();
		if ( index == -1 )
			return ResultErrorInvalidState;
		m_timer = & s_timers[index];
	}

	m_timer->m_mode = TM_BASIC;
	m_timer->m_action = action;
	m_timer->m_period = period * (TicksFreq() / (1 MEGA_D));

	TIMER_CntInitTypeDef conf;
	TIMER_CntStructInit(& conf);
	
	conf.TIMER_Prescaler = (mode == Microseconds ? 1 : 1 KILO_D);
	conf.TIMER_Period = m_timer->m_period;
	
	TIMER_CntInit(m_timer->m_handle, & conf);

	TIMER_BRGInit(m_timer->m_handle, TIMER_HCLKdiv1);

	TIMER_ITConfig(m_timer->m_handle, TIMER_STATUS_CNT_ARR, ENABLE);
	NVIC_EnableIRQ(m_timer->m_irq_n);

	return ResultOk;
}

Result Timer::DeInitialize()
{
	if ( m_timer && m_timer->m_mode != TM_INACTIVE ) {
		TIMER_DeInit(m_timer->m_handle);
		m_timer->m_mode = TM_INACTIVE;
	}
	return ResultOk;
}

Result Timer::Start(bool immediate_tick)
{
	if ( ! m_timer || m_timer->m_mode != TM_BASIC ) 
		return ResultErrorInvalidState;
	
	TIMER_SetCounter(m_timer->m_handle, immediate_tick ? m_timer->m_period - 1 : 0);  // TODO: проверить, можно ли без единицы
	
	TIMER_Cmd(m_timer->m_handle, ENABLE);

	return ResultOk;
}

Result Timer::Stop()
{
	if ( ! m_timer || m_timer->m_mode != TM_BASIC ) 
		return ResultErrorInvalidState;

	TIMER_Cmd(m_timer->m_handle, DISABLE);

	return ResultOk;
}

ulong Timer::GetTick()
{
	return TIMER_GetCounter(m_timer->m_handle);
}

void Timer::SetTick(ulong tick)
{
	TIMER_SetCounter(m_timer->m_handle, tick);
}

ulong Timer::TicksFreq()
{
	return System::GetCpuFreq();
}

ulong Timer::TicksMaxVal()
{
	return m_timer->m_period - 1;
}

// обработчики прерываний таймеров
extern "C" {
void TIMER1_IRQHandler()
{
	if ( s_timers[0].m_action ) 
		(* s_timers[0].m_action)();
	TIMER_ClearITPendingBit(MDR_TIMER1, TIMER_STATUS_CNT_ARR);
}
void TIMER2_IRQHandler()
{
	if ( s_timers[1].m_action ) 
		(* s_timers[1].m_action)();
	TIMER_ClearITPendingBit(MDR_TIMER2, TIMER_STATUS_CNT_ARR);
}
void TIMER3_IRQHandler()
{
	if ( s_timers[2].m_action ) 
		(* s_timers[2].m_action)();
	TIMER_ClearITPendingBit(MDR_TIMER3, TIMER_STATUS_CNT_ARR);
}
void TIMER4_IRQHandler()
{
	if ( s_timers[3].m_action ) 
		(* s_timers[3].m_action)();
	TIMER_ClearITPendingBit(MDR_TIMER4, TIMER_STATUS_CNT_ARR);
}
}

#endif	// #if MACS_USE_TIMERS
