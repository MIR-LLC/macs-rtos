/// @file macs_common.cpp
/// @brief Общие определения для кода ОС 
/// @copyright AstroSoft Ltd, 2016

#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include "macs_system.hpp"
#include "macs_memory_manager.hpp"
#include "macs_scheduler.hpp"
#include "macs_mutex.hpp"

uint32_t SystemBase::m_tick_rate_hz = MACS_INIT_TICK_RATE_HZ;

namespace macs {
	
#if MACS_DEBUG
void _Assert(bool e)
{
	if ( ! e ) 
		MACS_ALARM(AR_ASSERT_FAILED);
	return;
}
#endif

extern "C" CSPTR GetResultStr(Result retcode, bool brief)
{
	switch ( retcode ) {
	case ResultOk :                         return brief ? "Ok" : "Ok";
	case ResultTimeout :                    return brief ? "TOut"  : "Timeout";
	case ResultErrorInterruptNotSupported : return brief ? "InInt" : "Interrupt not supported";
	case ResultErrorSysCallNotAllowed :     return brief ? "SyCal" : "Sys call not allowed";
	case ResultErrorNotSupported :          return brief ? "NotSp" : "Not supported";
	case ResultErrorInvalidArgs :           return brief ? "InvAr" : "Invalid args";		
	case ResultErrorInvalidState :          return brief ? "InvSt" : "Invalid state";
	}
	return brief ? "Unkn" : "Unknown error";
}

int RandN(int n) // [1..n]
{
	return n * (rand() / ((double) RAND_MAX + 1)) + 1;
}

void Sprintf(char * buf, size_t bufsz, CSPTR format, ...)
{
	va_list args;
	va_start(args, format);
	int retval = vsnprintf(buf, bufsz, format, args);
	va_end(args);
	
	_ASSERT(retval >= 0);
	if ( retval >= bufsz ) 
		MACS_ALARM(AR_SPRINTF_TRUNC);
} 

char * PrnFmt::s_buf = nullptr;
static Mutex PrnFmtMutex;            
static Task * PrnFmtTask = nullptr;  

PrnFmt::PrnFmt(const char * format, ...) 
{
	Task * cur_task = Sch().GetCurrentTask();
	Task * prev_task = (Task *) ExclSetPtr((void * &) PrnFmtTask, cur_task);
	if ( ! cur_task || cur_task != prev_task ) {
		PrnFmtMutex.Lock();
		
		if ( ! s_buf ) 
			s_buf = new char[SPRINTF_BUFSZ];
		 
		va_list args;
		va_start(args, format);
		int retval = vsnprintf(s_buf, SPRINTF_BUFSZ, format, args);
		va_end(args);

		_ASSERT(retval >= 0);
		if ( retval >= SPRINTF_BUFSZ ) 
			MACS_ALARM(AR_SPRINTF_TRUNC);
	} else {
		MACS_ALARM(AR_DOUBLE_PRN_FMT);
		PrnFmtTask = nullptr;
	}
}
 
PrnFmt::~PrnFmt() 
{ 
	PrnFmtMutex.Unlock(); 
	PrnFmtTask = nullptr;
}  
 
CSPTR const g_zstr = ""; 
CSPTR const String::NEWLINE = "\r\n"; 
 
String & String::Add(CSPTR ptr, size_t len)
{
	if ( len ) {
		_ASSERT(ptr);	
		size_t old_len = Len();
		char * new_str = new char[old_len + len + 1];
		if ( old_len ) 
			memcpy(new_str, m_str, old_len);
		memcpy(new_str + old_len, ptr, len);
		new_str[old_len + len] = '\0';
		if ( m_str )
			delete[] m_str;
		m_str = new_str;
	}
	return * this;
}

CRC32 g_crc32;

CRC32::CRC32()
{
	loop ( int, i, countof(m_table) ) {
		uint32_t r = i, j = 8;
		while ( j -- )
			r = (r & 0x1) ? (r >> 1) ^ CRC_POLY : r >> 1;
		m_table[i] = r;
	}
}

uint32_t CRC32::Calc(const byte * ptr, size_t len, uint32_t crc)
{
	while ( len -- ) {
		crc = m_table[((byte) crc) ^ * ptr ++] ^ crc >> 8;
		crc ^= CRC_MASK;
	}
	return crc;
}
  
uint32_t MsToTicks(uint32_t ms)
{
	return (System::GetTickRate() * ms) / 1000;
}

uint32_t TicksToUs(uint32_t ticks)
{
	return ticks * (1000000 / System::GetTickRate());
}
	
}	// namespace macs

size_t StackPtr::GetVirginLen(uint32_t * beg, uint32_t * lim)
{	
	uint32_t * ptr = beg;
	while ( ptr < lim && * ptr == TOP_MARKER ) 
		++ ptr;
	return ptr - beg;
}


void StackPtr::FillWithMark(uint32_t * ptr, uint32_t * lim)
{
	while ( ptr < lim )
		* ptr ++ = TOP_MARKER;
}

bool TaskStack::Check()
{
	StackPtr::CHECK_RES res = m_top.Check(m_margin, m_len);
	
	if ( res == StackPtr::SP_UNDERFLOW ) {
		ALARM_ACTION act = MACS_ALARM(AR_STACK_UNDERFLOW);
		if ( act == AA_KILL_TASK ) 
			return false;
	}	else if ( res == StackPtr::SP_OVERFLOW )
		MACS_ALARM(AR_STACK_OVERFLOW);		
	else if ( res == StackPtr::SP_CORRUPTED ) 
		MACS_ALARM(AR_STACK_CORRUPTED);
	
	return true;
}

void TaskStack::Build(size_t len, uint32_t * mem)
{
	Free();
		
	if ( len ) {
		size_t guard 
#if MACS_MPU_PROTECT_STACK		
									= GUARD_SIZE;
#else		
									= (len > MIN_SIZE ? GUARD_SIZE : 0);
#endif
		m_is_alien_mem = mem;
		m_memory = mem;

		BuildPlatformSpecific(guard, len);

#if	MACS_WATCH_STACK
		m_top.Instrument(m_margin, true);
#else	
		m_top.Instrument(m_margin, false);
#endif		
	}	
}

void TaskStack::Prepare(size_t len, void * this_ptr, void (* run_func)(void), void (* exit_func)(void))
{
	if ( ! m_memory )
		Build(len);

	PreparePlatformSpecific(len, this_ptr, run_func, exit_func);
}

void TaskStack::Free() 
{ 
	if ( ! m_is_alien_mem && m_memory ) 
		delete [] m_memory; 
	m_is_alien_mem = false; 
	m_len = 0; 
	m_memory = nullptr; 
	m_margin.Zero();
	m_top.Zero();
}

namespace macs {
	
StackPtr SchedulerSwitchContext(StackPtr new_sp)
{
	return Sch().SwitchContext(new_sp);
}

byte ExclSet(byte & flag)
{
	uint8_t old_flag;
	do 
		old_flag = MACS_LDREXB(& flag);
	while ( MACS_STREXB(true, & flag) );
	__DMB();
	return old_flag;		
}

byte ExclIncCnt(byte & cnt)
{
	byte old_cnt, new_cnt;
	do {
		old_cnt = MACS_LDREXB(& cnt);
		new_cnt = old_cnt + 1;
	} while ( MACS_STREXB(new_cnt, & cnt) );
	__DMB();
	if ( new_cnt == 0 )
		MACS_ALARM(AR_COUNTER_OVERFLOW);
	return old_cnt;			
}

ulong ExclIncCnt(ulong & cnt)
{
	ulong old_cnt, new_cnt;
	do {
		old_cnt = MACS_LDREXW(& cnt);
		new_cnt = old_cnt + 1;
	} while ( MACS_STREXW(new_cnt, & cnt) );
	__DMB();
	if ( new_cnt == 0 )
		MACS_ALARM(AR_COUNTER_OVERFLOW);
	return old_cnt;		
}	

void * ExclSetPtr(void * & ptr, void * new_val)
{
	void * old_val;
	ulong new_val_ = (ulong) new_val;
	do 
		old_val = (void *) MACS_LDREXW((ulong *) & ptr);
	while ( MACS_STREXW(new_val_, (ulong *) & ptr) );
	__DMB();
	return old_val;		
}	

long ExclChg(long & val, long chg)
{
	long res;
	do {
		res = MACS_LDREXW((ulong *) & val);
		res += chg;
	} while ( MACS_STREXW(res, (ulong *) & val) );
	__DMB();
	return res;		
}	

void SpinLock::Lock()	
{
	do 
		while ( MACS_LDREXW((ulong *) & m_lockVar) ) 
			MacsCpuDelay(1);
	while ( MACS_STREXW(1, (ulong *) & m_lockVar) );
	__DMB();
}
void SpinLock::Unlock()	{ __DMB(); m_lockVar = 0; }

}	// namespace macs

ulong SystemBase::AskCurCpuTick()
{
	return IsInPrivOrIrq() ? GetCurCpuTick() : SvcExecPrivileged(0, 0, 0, EPM_Read_Cpu_Tick);
}

