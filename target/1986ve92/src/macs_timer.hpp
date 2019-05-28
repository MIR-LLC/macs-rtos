/** @copyright AstroSoft Ltd */
#pragma once

#include "common.hpp"

class TimerAction
{
public:
	virtual void operator()() = 0;
};

struct TimerData;

class Timer
{
public:
	Timer();
	~Timer();

	enum MeasureMode
	{
		Microseconds,
		Milliseconds
	};

	Result Initialize(uint period, MeasureMode mode, TimerAction * action);
	Result DeInitialize();
	Result Start(bool immediate_tick = false);
	Result Stop();

	ulong GetTick();
	void SetTick(ulong);
	ulong TicksFreq();
	ulong TicksMaxVal();

private:
	TimerData * m_timer;
};
