/// @file macs_profiler.cpp
/// @brief Профилировщик.
/// @details Предоставляет интерфейс к профилировщику, способному точно измерять временные характеристики приложения.
/// Профилировщик учитывает собственную задержку, так что ее величина не влияет на результаты измерений.
/// Для использования профилировщика необходимо включить опцию MACS_PROFILING_ENABLED в настройках системы.
/// @copyright AstroSoft Ltd, 2016
 
#include "macs_tunes.h"

#if MACS_PROFILING_ENABLED 

#include "macs_common.hpp"
#include "macs_critical_section.hpp"
#include "macs_task.hpp"
#include "macs_profiler.hpp"
 
namespace performance {
	
ProfData g_prof_data[PE_QTTY];
static bool ProfDummy;

tick_t ProfData::s_empty_call_overhead = 0;
tick_t ProfData::s_empty_constr_overhead = 0;
tick_t ProfData::s_embrace_overhead = 0;
	 
ProfEye * ProfEye::s_cur_eye = nullptr;

ProfEye::ProfEye(PROF_EYE eye, bool run)
{ 
	m_eye = eye; 
	m_lost = 0;
	m_run = false; 
	if ( run ) { 
		{ CriticalSection _cs_;
			g_prof_data[m_eye].Lock(true);
			m_up_eye = s_cur_eye; 
			s_cur_eye = this;
		}
		Start(); 
	} else 
		m_up_eye = nullptr; 
}	

ProfEye::~ProfEye() 
{ 
	if ( m_run ) 
		Stop(false); 
	if ( s_cur_eye == this ) { 
		CriticalSection _cs_;
		g_prof_data[m_eye].Lock(false);
		s_cur_eye = m_up_eye;	
	}
} 

void ProfEye::Start() { 
	if ( m_run ) { 
		ProfDummy = ! ProfDummy;	// Чтобы избежать оптимизации при отключенном режиме отладки
		_ASSERT(false); 
	} 
	m_start = System::GetCurCpuTick(); 
	m_run = true; 
}	

void ProfEye::Stop(bool call)
{
	tick_t tot_time = System::GetCurCpuTick() - m_start;
	m_lost += (call ? ProfData::s_empty_call_overhead : ProfData::s_empty_constr_overhead);
	long pure_time = (long) tot_time - m_lost;

	if ( ! m_run ) {
		ProfDummy = ! ProfDummy;	// Чтобы избежать оптимизации при отключенном режиме отладки 
		_ASSERT(false); 
	} 
	m_run = false;
	
	CriticalSection _cs_;
	
	if ( m_up_eye )	// Всегда true кроме секций верхнего уровня
		m_up_eye->m_lost += m_lost + ProfData::s_embrace_overhead;
	 
	g_prof_data[m_eye].m_time += pure_time;
	g_prof_data[m_eye].m_lost += m_lost;
	g_prof_data[m_eye].m_sqrs += pure_time * (int64_t) pure_time;
	
	// Все ветки должны иметь одно и то же время выполнения
	long min = MIN(pure_time, g_prof_data[m_eye].m_min);	
	g_prof_data[m_eye].m_min = min;	 
	long max = MAX(pure_time, g_prof_data[m_eye].m_max);	
	g_prof_data[m_eye].m_max = max;
	
	++ g_prof_data[m_eye].m_cnt; 
	
	m_lost = 0;
}

static CSPTR EyeName(PROF_EYE eye)
{
	switch ( eye ) {
	case PE_EMPTY_CALL : 		return "EmptyCall"; 
	case PE_EMPTY_CONSTR : 	return "EmptyConstr"; 
	case PE_EMBRACE : 			return "Embrace"; 

	case PE_ZERO_CALL_A1 : 				return "zCall_A1"; 
	case PE_ZERO_CALL_B1 : 				return "zCall_B1"; 
	case PE_ZERO_CONSTR_A1 : 			return "zCon_A1"; 
	case PE_ZERO_CONSTR_B1 : 			return "zCon_B1"; 
	case PE_ZERO_CONSTR_B1_2 : 		return "zCon_B1_2"; 
	case PE_ZERO_CONSTR_C1 : 			return "zCon_C1"; 
	case PE_ZERO_CONSTR_C1_2 : 		return "zCon_C1_2"; 
	case PE_ZERO_CONSTR_D1_2 : 		return "zCon_D1_2"; 
	case PE_ZERO_CONSTR_D1_2_3 :	return "zCon_D1_2_3"; 
				
	case PE_INCR_INT : 			return "IncrInt"; 
		
	case PE_CRIT_SEC_INT_ENTR : return "CrSecIntEntr"; 
	case PE_CRIT_SEC_INT_EXIT : return "CrSecIntExit"; 
	case PE_CRIT_SEC_EXT_ENTR : return "CrSecExtEntr"; 
	case PE_CRIT_SEC_EXT_EXIT : return "CrSecExtExit"; 

	case PE_MEM_ALLOC :	return "MemAlloc";
	case PE_MEM_FREE :	return "MemFree";
			
	case PE_TASK_INIT	:	return "TaskInit";
	case PE_TASK_ADD :	return "TaskAdd";
	case PE_TASK_DEL :	return "TaskDel";
	
	case PE_IQR_HANDLE : return "IrqHandle"; 

	case PE_EVENT_INIT : 		return "EventInit"; 
	case PE_EVENT_RAISE : 	return "EventRaise"; 
	case PE_EVENT_ACTION :	return "EventAction"; 
		
	case PE_MUTEX_INIT :		return "MutexInit";
	case PE_MUTEX_LOCK :		return "MutexLock";
	case PE_MUTEX_UNLOCK :	return "MutexUnlock";
	case PE_MUTEX_ACTION :	return "MutexAction";

	case PE_SEMPH_INIT : 		return "SemphInit"; 
	case PE_SEMPH_GIVE : 		return "SemphGive"; 
	case PE_SEMPH_TAKE : 		return "SemphTake"; 
	case PE_SEMPH_ACTION :	return "SemphAction"; 

	case PE_DELAY_10MS :	return "Delay10ms";
	}
	return nullptr;	
}

static void PrintEyeName(String & str, PROF_EYE eye)
{
	CSPTR name = EyeName(eye);
	str << (name ? PrnFmt("%12s:  ", name) : PrnFmt("N=%10d:  ", (int) eye));
}

void ProfEye::Tune()
{
	ProfData::s_empty_call_overhead = 0;
	ProfData::s_empty_constr_overhead = 0;
	ProfData::s_embrace_overhead = 0;
	
	CriticalSection _cs_;
	
	loop ( int, i, 1000 ) {		
		PROF_DECL(PE_EMPTY_CALL, empty_call);
		PROF_START(empty_call);
		PROF_STOP(empty_call);
		
		{ PROF_EYE(PE_EMBRACE, _embrace_); 
			{ PROF_EYE(PE_EMPTY_CONSTR, _empty_constr_); 
			}	
		}	
	}
	ProfData::s_empty_call_overhead = g_prof_data[PE_EMPTY_CALL].TimeAvg();
	ProfData::s_empty_constr_overhead = g_prof_data[PE_EMPTY_CONSTR].TimeAvg();
	ProfData::s_embrace_overhead = g_prof_data[PE_EMBRACE].TimeAvg() + ProfData::ADJUSTMENT - 2 * ProfData::s_empty_constr_overhead;
}

void ProfEye::Print(String & str, bool brief, bool use_ns) 
{ 
	if ( ! brief )
		PrintEyeName(str, m_eye);
	g_prof_data[m_eye].Print(str, brief, use_ns);
}

void ProfEye::PrintResults(String & str, bool brief, bool use_ns)
{
	str.Add("Profiler statistics:\n\r");
	loop ( int, i, PE_QTTY ) {
		PrintEyeName(str, (PROF_EYE) i);
		g_prof_data[i].Print(str, brief, use_ns);

		if ( i == PE_EMBRACE )
			str << "------------" << String::NEWLINE;
	}
	str.NewLine(); 
}

void ProfData::Print(String & str, bool brief, bool use_ns)
{
	if ( ! brief ) {
		str << PrnFmt("Cnt=%-8ld  ");
		str << (! use_ns ? PrnFmt("TTot=%-8ld  TOvh=%-8ld  ", TimeTot(), TimeOvh())
		                 : PrnFmt("TTot(ns)=%-8ld  TOvh(ns)=%-8ld  ", System::CpuTicksToNs(TimeTot()), System::CpuTicksToNs(TimeOvh())));
	}

	str << (! use_ns ? PrnFmt("TMin=%-8ld  TMax=%-8ld  TDev=%-8ld  TAvg=%-8ld\r\n", 
	                          TimeMin(), TimeMax(), TimeDev(), TimeAvg())
	                 : PrnFmt("TMin(ns)=%-8ld  TMax(ns)=%-8ld  TDev(ns)=%-8ld  TAvg(ns)=%-8ld\r\n", 
	                          System::CpuTicksToNs(TimeMin()), System::CpuTicksToNs(TimeMax()), 
	                          System::CpuTicksToNs(TimeDev()), System::CpuTicksToNs(TimeAvg())));
}

}	// namespace performance 

#endif	/* #if MACS_PROFILING_ENABLED  */
