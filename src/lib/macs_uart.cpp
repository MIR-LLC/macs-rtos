/// @file macs_uart.cpp
/// @brief Порт для интерфейса UART.
/// @details Реализация универсального порта для интерфейса UART.
/// @copyright AstroSoft Ltd, 2016

#include "macs_common.hpp"

#if MACS_USE_UART

#include "macs_uart.hpp"
#include "macs_uart_adapter.hpp"
#include "macs_scheduler.hpp"
	
namespace utils {

void InitUartDrv()
{
	static bool s_is_ready = false;
	if ( ! s_is_ready ) {	
		Uart_InitDrv();
		s_is_ready = true;	
	}
}

static const int UartTaskIrq = FIRST_VIRT_IRQ + 1;
	
PortUart g_uart_port;

void Uart_OnSend(UartHandler)
{
	g_uart_port.OnSend();
}
 
void Uart_OnRecv(UartHandler, size_t len)
{
	_ASSERT(g_uart_port.IsOpened());
	//g_uart_port.OnRecvBuf(len);
	Sch().ProceedIrq(UartTaskIrq);
}
 
bool PortUart::Open(const PortConfig * config) 
{		
	bool retcode = BufferedPort::Open(config);	RET_ERROR(retcode, false);
	const PortUartConfig conf, * pc;
	m_speed_bps = (config ? config->m_speed_bps : conf.m_speed_bps);
	pc = (config && ! config->m_is_base ? (const PortUartConfig *) config : & conf);
	m_word_length = pc->m_word_length;
	m_stop_bits = pc->m_stop_bits;
	m_parity = pc->m_parity;			

	m_uart_hndl = Uart_Open(pc->m_num, m_speed_bps, m_word_length, m_stop_bits, m_parity); 								
	RET_ERROR(m_uart_hndl != INVALID_UART_HANDLER, false);
	 
	/*	
	if ( m_recv_hndl ) {
		Result res = TaskIrq::Add(m_task = new PortUartTask(* this), UartTaskIrq, Task::PriorityHigh);	RET_ASSERT(res == ResultOk, false);
	}
	*/
	return true;
}

bool PortUart::Close()
{
	if ( IsOpened() ) {
		_ASSERT(m_uart_hndl != INVALID_UART_HANDLER);
		Uart_Close(m_uart_hndl);
		m_uart_hndl = INVALID_UART_HANDLER;
		
		/*
		if ( m_recv_hndl ) {
			_ASSERT(m_task);		
			m_task->Delete();
			m_task = nullptr;
		}
		*/
		
		BufferedPort::Close();
	}
	return true;
}

bool PortUart::Require(size_t len) 
{ 
	while ( ! MayRead() ) 
		Task::Delay(1);

	m_buffer.Alloc(len);	 
	Result retcode = Uart_RecvSemph(m_uart_hndl, m_buffer, len, INFINITE_TIMEOUT);			
	RET_ERROR(retcode == ResultOk, false);	
	Uart_OnRecv(m_uart_hndl, len);
	return true;

	//Result retcode = Uart_RecvIrq(m_uart_hndl, m_buffer, len); 
	//return retcode == ResultOk;
}

Result PortUart::SendData(SendMode mode, const byte * ptr, size_t len, ulong timeout_ms) 
{
	if ( ! MayWrite() ) 
		return ResultErrorInvalidState;

	DynBuf buf;
	buf.Dupe((byte *) ptr, len);  // TODO: для совместимости с версией MacsBuffer ниже 58361
	Result retcode = mode.Check(SM_USE_IRQ) ?	
										Uart_SendIrq(m_uart_hndl, buf) :					
										Uart_SendWait(m_uart_hndl, buf, timeout_ms);	
	
	RET_ERROR(retcode == ResultOk, retcode);
	
	return mode.Check(SM_USE_IRQ) ? m_send_semph.Wait(timeout_ms) : ResultOk;
} 
 
Result PortUart::RecvData(RecvMode mode, Buf & buf, size_t len, ulong timeout_ms)
{
	while ( ! MayRead() ) 
		Task::Delay(1);
	
	buf.Alloc(len);	 
	Result retcode;
	
	if ( mode.Check(RM_USE_SEMPH) )
		retcode = Uart_RecvSemph(m_uart_hndl, buf, len, timeout_ms);		
	else
		retcode = Uart_RecvWait(m_uart_hndl, buf, len, timeout_ms);		 
	RET_ERROR(retcode == ResultOk, retcode);	
	return ResultOk;
}  

}	// namespace utils

#endif	/* #if MACS_USE_UART */


