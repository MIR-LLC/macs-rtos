/// @file macs_uart.hpp
/// @brief Порт для интерфейса UART.
/// @details Реализация универсального порта для интерфейса UART.
/// @copyright AstroSoft Ltd, 2016

#pragma once
 
#include "macs_common.hpp"

#if MACS_USE_UART

#include "macs_uart_adapter.hpp"
#include "macs_port.hpp"
#include "macs_semaphore.hpp"
  
namespace utils { 
 
class PortUartConfig : public PortConfig
{
public:
	short  m_num;	// -1 - очередной свободный, иначе - номер по порядку, заданному в адаптере
	ushort m_word_length;
	ushort m_stop_bits;
	ushort m_parity;	// 0 - none, 1 - odd, 2 - even
public:
	PortUartConfig() { 
		m_is_base = false; 		
		if ( m_speed_bps == ULONG_MAX )
			m_speed_bps = 115200;
		m_num = -1;
		m_word_length = 8;  		
		m_stop_bits = 1;
		m_parity = 0;
	}
};

class PortUartTask;

/// @brief Интерйейс для управление универсальным портом, работающим через UART-интерфейс.
class PortUart : public DefBufferedPort
{
private:
	UartHandler m_uart_hndl;
	PortUartTask * m_task;
	Semaphore m_send_semph;
protected:
	ushort m_word_length;
	ushort m_stop_bits;
	ushort m_parity;	// 0 - none, 1 - odd, 2 - even
public:	
	/// @brief Открывает порт с указанной конфигурацией.
	/// @param config Параметры конфигурации порта.
	/// @return true - если порт успешно открыт, false - в противном случае.
	virtual bool Open(const PortConfig * config = nullptr);
	/// @brief Закрывает порт.
	/// @return true - если порт успешно закрыт, false - в противном случае.
	virtual bool Close();

// Под Lynx нужно продублировать описания виртуальных методов, иначе вылетаем по нулевому адресу.
#ifdef __CMLYNX__
	virtual bool ChangeState(STATE state, bool set) { return DefBufferedPort::ChangeState(state, set); }
 
	virtual Result Flush(ulong timeout_ms = INFINITE_TIMEOUT) { return ResultOk; } 

	virtual Result Send(SendMode mode, const byte *ptr, size_t len, ulong timeout_ms = INFINITE_TIMEOUT)
		{ return SendData(mode, ptr, len, timeout_ms); }
	Result Send(SendMode mode, const Buf &buf, ulong timeout_ms = INFINITE_TIMEOUT)
		{ return Send(mode, buf.Data(), buf.Len(), timeout_ms); }
	Result Send(const byte *ptr, size_t len, ulong timeout_ms = INFINITE_TIMEOUT)
		{ return Send(m_def_send_mode, ptr, len, timeout_ms); }
	Result Send(const Buf &buf, ulong timeout_ms = INFINITE_TIMEOUT)
		{ return Send(m_def_send_mode, buf, timeout_ms); }

	virtual Result Receive(RecvMode mode, Buf &buf, size_t len, ulong timeout_ms)
		{ return RecvData(mode, buf, len, timeout_ms); }
	Result Receive(RecvMode mode, Buf &buf, size_t len)
		{ return RecvData(mode, buf, len, INFINITE_TIMEOUT); }
	Result Receive(Buf &buf, size_t len, ulong timeout_ms = INFINITE_TIMEOUT)
		{ return Receive(m_def_recv_mode, buf, len, timeout_ms); }
#endif

	virtual bool Require(size_t len); 

	/// @brief Функция обратного вызова, вызываемая после отправки сообщения через порт.
	void OnSend() {
		m_send_semph.Signal();
	}		 

	/// @brief Функция обратного вызова, вызываемая перед считыванием из порта указанного количества байт.
	/// @param len Количество байт.
	void OnRecvBuf(size_t len) {
		m_buffer.AddLen(len);
	}		

	/// @brief Функция обратного вызова, вызываемая после получения данных.
	void OnRecv() {
		/*
		if ( m_recv_hndl ) 
			(* m_recv_hndl)(m_buffer);
		*/
	}		
protected:
	virtual Result SendData(SendMode mode, const byte * ptr, size_t len, ulong timeout_ms);
	virtual Result RecvData(RecvMode mode, Buf & buf, size_t len, ulong timeout_ms);	
};
extern PortUart g_uart_port;
	   
/// @brief Класс, осуществляющий приём сообщений через UART-интерфейс в отдельном потоке.
/// @details При добавлении экземпляра этого класса в планировщик следует указать номер прерывания, источником которого является соответствующий модуль UART 
class PortUartTask : public TaskIrq
{
private:
	PortUart & m_port;
public:
	/// @brief Конструктор. Создает объект, осуществляющий оповещение о получении данных через указанный UART-порт.
	/// @param port UART-порт.
	PortUartTask(PortUart & port) : TaskIrq("UART"), m_port(port) {}

	/// @brief Функция обработки прерывания.
	virtual void IrqHandler() {
		m_port.OnRecv();
	}
};
 
}	// namespace utils

using namespace utils;

#endif	/* #if MACS_USE_UART */
