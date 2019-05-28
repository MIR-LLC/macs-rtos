#include "macs_term_cmd.hpp"

#if MACS_USE_TERMINAL

#include <stdlib.h>

#include "macs_log.hpp"

namespace utils {

HelpTermCmd::HelpTermCmd(const TermCommands & cmds) : TermCommand("Получение справки"), m_cmds(cmds) {}	
void HelpTermCmd::DoAction(Terminal & term, const DynArr<CSPTR> & args)
{
	term.WriteLine("Команды:");
	loop ( uint, index, m_cmds.Count() )
		term.WriteLine(PrnFmt("  %s - %s", ZSTR(m_cmds[index].m_name), ZSTR(m_cmds[index].m_cmd->m_comment)));
}
 
ContextSwitchTemrCmd::ContextSwitchTemrCmd() : TermCommand("Измерение времени переключения контекста") {}
void ContextSwitchTemrCmd::DoAction(Terminal & term, const DynArr<CSPTR> & args)
{
	extern ulong MeasureContextSwitchTime();
	term.WriteLine("Running...");
	// Эта функция находится в тестах
	ulong swc_ns = 0;  // MeasureContextSwitchTime();
	term.WriteLine(PrnFmt("Context switch (ns): %ld", swc_ns));
}
ContextSwitchTemrCmd g_ctx_swc_tc;
	
TickRateTemrCmd::TickRateTemrCmd() : TermCommand("Установка частоты тиков ОС") {}
void TickRateTemrCmd::DoAction(Terminal & term, const DynArr<CSPTR> & args)
{
	if ( args.Count() != 1 ) {
		term.WriteLine("Использование: rate rate_hz");
		return;
	}			
		
	if ( ! System::SetTickRate(atoi(args[0])) ) {
		term.WriteLine("Ошибка при выполнении");
		return;
	}			
}
TickRateTemrCmd g_tickrate_tc;

TaskListTemrCmd::TaskListTemrCmd() : TermCommand("Получение информации о выполняющихся задачах") {}
void TaskListTemrCmd::DoAction(Terminal & term, const DynArr<CSPTR> & args)
{
	String str;
	DynArr<TaskInfo> info;
	Sch().GetTasksInfo(info);
	TaskInfo::PrintHeader(str);
	str.Add("\r\n----------------------------------------------------------------\r\n");
	loop ( uint, index, info.Count() ) {
		info[index].Print(str);
		str.Add("\r\n");
	}
	term.WriteLine((CSPTR) str);
}
TaskListTemrCmd g_tlist_tc;

#if MACS_USE_LOG
SysLogTemrCmd::SysLogTemrCmd() : TermCommand("Просмотр системного журнала") {}
void SysLogTemrCmd::DoAction(Terminal & term, const DynArr<CSPTR> & args)
{
	String str;
	g_sys_log.Print(str);
	term.WriteLine((CSPTR) str);
}
SysLogTemrCmd g_syslog_tc;
#endif

}	// namespace utils
 
#endif	// #if MACS_USE_TERMINAL
