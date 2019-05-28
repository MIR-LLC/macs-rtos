/// @file macs_clock.cpp
/// @brief Метки времени.
/// @details Предоставляет классы для работы с системными часами.
/// @copyright AstroSoft Ltd, 2015-2016

#include "macs_tunes.h"

#if MACS_USE_CLOCK

#include "macs_clock.hpp"
#include "macs_scheduler.hpp"

namespace utils {

char Time::s_prn_buf[] = "000d00h00m00s.000";

CSPTR Time::ToStr(bool verb) const
{
	verb ? Sprintf(s_prn_buf, sizeof(s_prn_buf), "%03dd%02dh%02dm%02ds.%03d", GetD(), GetH(), GetM(), GetS(), GetMs())
	     : Sprintf(s_prn_buf, sizeof(s_prn_buf), "%01d:%02d.%03d", GetM(), GetS(), GetMs());
	return s_prn_buf;
}

uint32_t Clock::s_cur_scnd = 0;
uint32_t Clock::s_last_frac = 0;
uint32_t Clock::s_last_scnd_tick = 0;

void Clock::OnTick(uint32_t tick)
{
	uint32_t spend = tick - s_last_scnd_tick;
	if ( spend >= System::GetTickRate() ) {
		s_cur_scnd += spend / System::GetTickRate();	// В принципе можно допустить ситуацию, когда планировщик стоял больше одной секунды.		
		s_last_scnd_tick = tick;
		if ( System::IsInPrivOrIrq() )
			s_last_frac = System::GetCurCpuTick();
		else
			s_last_frac += System::GetCpuFreq();	// Редкий случай - вызов из паузы после долгого ожидания
		if ( spend > System::GetTickRate() ) {
			uint32_t odds = spend % System::GetTickRate();
			s_last_scnd_tick -= odds;
			s_last_frac -= odds * (System::GetCpuFreq() / System::GetTickRate());
		}
	}
}

void Clock::GetTime(Time & time)
{
	PauseSection _ps_;

	time.m_scnd = s_cur_scnd;
	time.m_frac = System::IsInPrivOrIrq() ? System::GetCurCpuTick() - s_last_frac
	                                      : (Sch().GetTickCount() - s_last_scnd_tick) * (System::GetCpuFreq() / System::GetTickRate());			
	time.Norm();
}

}	// namespace utils

#endif	// #if MACS_USE_CLOCK
