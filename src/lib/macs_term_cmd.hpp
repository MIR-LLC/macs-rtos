#pragma once

#include "macs_terminal.hpp"

#if MACS_USE_TERMINAL

namespace utils {
	
class HelpTermCmd : public TermCommand
{
private:	
	const TermCommands & m_cmds;
public:
	HelpTermCmd(const TermCommands & cmds);
	virtual void DoAction(Terminal & term, const DynArr<CSPTR> & args);
};

class ContextSwitchTemrCmd : public TermCommand
{
public:
	ContextSwitchTemrCmd();
	virtual void DoAction(Terminal & term, const DynArr<CSPTR> & args);
};
extern ContextSwitchTemrCmd g_ctx_swc_tc;

class TickRateTemrCmd : public TermCommand
{
public:
	TickRateTemrCmd();
	virtual void DoAction(Terminal & term, const DynArr<CSPTR> & args);
};
extern TickRateTemrCmd g_tickrate_tc;
 
class TaskListTemrCmd : public TermCommand
{		
public:
	TaskListTemrCmd();
	virtual void DoAction(Terminal & term, const DynArr<CSPTR> & args);
};
extern TaskListTemrCmd g_tlist_tc;

#if MACS_USE_LOG
class SysLogTemrCmd : public TermCommand
{
public:
	SysLogTemrCmd();
	virtual void DoAction(Terminal & term, const DynArr<CSPTR> & args);
};
extern SysLogTemrCmd g_syslog_tc;
#endif


} //  namespace utils
using namespace utils;
 
#endif	// #if MACS_USE_TERMINAL
