/// @file macs_log.cpp
/// @brief Журнал событий.
/// @details Предоставляет классы для работы с журналом событий.
/// @copyright AstroSoft Ltd, 2015-2016

#include "macs_tunes.h"

#if MACS_USE_LOG

#include "macs_log.hpp"

namespace utils {

String & Log::Print(String & str) const
{
	for ( const LogEvent * pe = g_sys_log.GetFirstEvent(); pe; pe = g_sys_log.GetNextEvent(pe) ) {
		String temp_str;
		pe->Print(temp_str);
		str.Add(temp_str).Add("\r\n");
	}
	return str;
}

Log g_sys_log;

LogOsEvent::OsEventsReg LogOsEvent::s_os_events_reg = OS_STARTED;

String & LogOsEvent::Print(String & str) const
{
	LogEvent::Print(str).Add(" ");

	switch ( m_kind ) {
	case OS_STARTED 	: str.Add("OS started");    break;
	case TASK_ADDED 	: str.Add("Task Added");    break;
	case TASK_REMOVED	: str.Add("Task removed");  break;
	}
	if ( !! m_task_name )
		str.Add(" ").Add(m_task_name);

	return str;
}

}	// namespace utils

#endif	// #if MACS_USE_CLOCK
