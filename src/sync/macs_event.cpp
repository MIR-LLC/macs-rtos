/// @file macs_event.cpp
/// @brief События.
/// @details События могут быть использованы в тех случаях, когда необходимо заблокировать
/// задачу до тех пор, пока не возникнет какое-либо условие для её разблокировки. 
/// @copyright AstroSoft Ltd, 2016

#include "macs_event.hpp"
#include "macs_critical_section.hpp"

namespace macs {

Event::Event(bool broadcast) :
	m_broadcast(broadcast)
{}

Event::~Event()
{}

Result Event::Raise()
{
	if ( ! Sch().IsInitialized() || ! Sch().IsStarted() )
		return ResultErrorInvalidState;

	if ( ! System::IsSysCallAllowed() )
		return ResultErrorSysCallNotAllowed;

	return System::IsInPrivOrIrq() ? Raise_Priv(this) 
	                               : SvcExecPrivileged(this, NULL, NULL, EPM_Event_Raise_Priv);
}

Result Event::Raise_Priv(Event * event)
{
	CriticalSection _cs_;

	while ( event->IsHolding() )	{
		event->UnblockTask();

		if ( ! event->m_broadcast )
			break;
	}

	return ResultOk;
}

Result Event::Wait(uint32_t timeout_ms)
{
	if ( ! Sch().IsInitialized() || ! Sch().IsStarted() )
		return ResultErrorInvalidState;

	if ( System::IsInInterrupt() )
		return ResultErrorInterruptNotSupported;


	Result res = System::IsInPrivOrIrq() ? Wait_Priv(this, timeout_ms)
	                                     : SvcExecPrivileged(this, reinterpret_cast<void*>(timeout_ms), NULL, EPM_Event_Wait_Priv);
	if ( ResultOk != res )
		return res;

	return Task::GetCurrent()->m_unblock_reason == Task::UnblockReasonTimeout ? ResultTimeout : ResultOk; 
}

Result Event::Wait_Priv(Event * event, uint32_t timeout_ms)
{
	CriticalSection _cs_;

	if ( timeout_ms == 0 )
		return ResultTimeout;

	return event->BlockCurTask(timeout_ms);
}

}	// namespace macs
