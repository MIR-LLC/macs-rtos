#pragma once

#include "macs_tunes.h"

#if MACS_USE_UART

#include "macs_buffer.hpp"

namespace utils {
	 
typedef uint32_t UartHandler;

const UartHandler INVALID_UART_HANDLER = (UartHandler) ~0x0;

/**************************  Инициализация драйвера  **************************/
extern void Uart_InitDrv();
  
extern UartHandler Uart_Open(short num, ulong speed_bps, ushort word_length, ushort stop_bits, ushort parity);
extern Result			 Uart_Close(UartHandler);

extern Result Uart_SendWait	(UartHandler, const Buf &, ulong timeout_ms); 
extern Result Uart_SendIrq	(UartHandler, const Buf &); 

extern Result Uart_RecvWait	(UartHandler, Buf &, size_t len, ulong timeout_ms); 
extern Result Uart_RecvIrq	(UartHandler, Buf &, size_t len); 
extern Result Uart_RecvSemph(UartHandler, Buf &, size_t len, ulong timeout_ms);	

extern void Uart_OnSend(UartHandler);
extern void Uart_OnRecv(UartHandler, size_t len);
	
}	// namespace utils

using namespace utils;

#endif	// #if MACS_USE_UART
