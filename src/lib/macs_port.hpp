/// @file macs_port.hpp
/// @brief Универсальный порт.
/// @details Концепция универсального порта, способного работать в разных режимах для разных реализаций. 
/// @copyright AstroSoft Ltd, 2015-2016

#pragma once
 
#include "macs_common.hpp"
#include "macs_buffer.hpp"
#include "macs_mutex.hpp"
  
namespace utils {
	
class PortBase 
{
public:
	/// @brief Режим работы порта.
	enum MODE {
		PM_ZERO = 0x00,
		PM_READ_ALLOWED = (0x01 << 0),                        ///< Разрешено чтение из порта
		PM_WRITE_ALLOWED = (0x01 << 1),                       ///< Разрешена запись в порт
		PM_RW_ALLOWED = (PM_READ_ALLOWED | PM_WRITE_ALLOWED)  ///< Разрешены чтение и запись
	};
};	

/// @brief Базовый класс, представляющий конфигурацию порта.
class PortConfig
{
public:
	/// @brief Флаг, указывающий на то, является ли данный экземпляр класса базовым.
	/// В конструкторе наследников флагу необходимо присвоить значение false.
	bool m_is_base;

	/// @brief Режим работы порта.
	PortBase::MODE m_mode;

	/// @brief Скорость порта.
	ulong m_speed_bps;

public:
	PortConfig(const PortConfig *config = nullptr)
	{
		m_is_base = true;
		if (config)
		{
			m_mode = config->m_mode;
			m_speed_bps = config->m_speed_bps;
		}
		else
		{
			m_mode = PortBase::PM_RW_ALLOWED;
			m_speed_bps = ULONG_MAX;
		}
	}
};


/// @brief Интерфейс для управления универсальным портом.
class Port : public PortBase
{
public:

	/// @brief Состояние порта.
	enum STATE {
		PS_ZERO = 0x00,
		PS_OPENED = (0x01 << 0),                              ///< Порт открыт
		PS_READ_ALLOWED = (0x01 << 1),                        ///< Разрешено чтение из порта
		PS_WRITE_ALLOWED = (0x01 << 2),                       ///< Разрешена запись в порт
		PS_RW_ALLOWED = (PS_READ_ALLOWED | PS_WRITE_ALLOWED), ///< Разрешены чтение и запись
		PS_READ_BUSY = (0x01 << 3),                           ///< Порт сейчас обрабатывает операцию чтения
		PS_WRITE_BUSY = (0x01 << 4),                          ///< Порт сейчас обрабатывает операцию записи
		PS_DATA_READY = (0x01 << 5)                           ///< Данные приняты и готовы к выдаче	
	};

	/// @brief Режим посылки данных.
	enum SEND_MODE {
		SM_ZERO 		= 0x00,
		SM_USE_IRQ 	= (0x01 << 0)
	};
	typedef BitMask<SEND_MODE> SendMode;

	/// @brief Режим приема данных.
	enum RECV_MODE {
		RM_ZERO 			= 0x00,
		RM_ACT_WAIT		= (0x01 << 0),	///< Использовать активное ожидание
		RM_USE_SEMPH	= (0x01 << 1),	///< Использовать семафор для ожидания
	};
	typedef BitMask<RECV_MODE> RecvMode;

private:
	MODE m_mode;
	BitMask<STATE> m_state;

protected:
	ulong m_speed_bps;
	SendMode m_def_send_mode;
	RecvMode m_def_recv_mode;

public:
	Mutex m_oper_mut;

public:
	/// @brief Конструктор. 
	Port() :
		m_oper_mut(true)
	{
		m_mode = PM_ZERO;
		m_speed_bps = ULONG_MAX;
	}

	/// @brief Возвращает режим работы порта.
	/// @return Режим работы порта.
	MODE Mode() { return m_mode; }

	/// @brief Возвращает состояние порта.
	/// @return Состояние порта.
	BitMask<STATE> State() { return m_state; }

	/// @brief Открывает порт в указанной конфигурации.
	/// @param config Параметры конфигурации порта.
	/// @return true - если порт успешно открыт, false - в противном случае.
	virtual bool Open(const PortConfig * config)
	{
		if (IsOpened())
		{
			bool retcode = Close();
			if (!retcode)
				return false;

			bool is_opened = m_state.CheckAny(PS_OPENED);
			_ASSERT(!is_opened);
			if (is_opened) 
				return false;
		}

		m_mode = (config ? config->m_mode : PM_RW_ALLOWED);
		if (m_mode & PM_READ_ALLOWED)	m_state.Add(PS_READ_ALLOWED);
		if (m_mode & PM_WRITE_ALLOWED)	m_state.Add(PS_WRITE_ALLOWED);

		m_speed_bps = (config ? config->m_speed_bps : ULONG_MAX);
		m_state.Add(PS_OPENED);

		return true;
	}
	bool Open() { return Open(nullptr); }  // Обход проблемы с виртуальными методами в CMLYNX


	/// @brief Закрывает порт.
	/// @return true - если порт успешно закрыт, false - в противном случае.
	virtual bool Close()
	{
		if (IsOpened())
		{
			m_mode = PM_ZERO;
			m_state.Set(PS_ZERO);
		}
		return true;
	}

	/// @brief Модифицирует состоние порта.
	/// @param state Состояние порта.
	/// @param set Флаг: true - перезаписать полностью состояние порта, false - перезаписать только указанные биты.
	/// @return true - если состояние порта успешно обновлено, false - в противном случае.
	virtual bool ChangeState(STATE state, bool set)
	{
		set ? m_state.Add(state) : m_state.Rem(state);
		return true;
	}

	/// @brief Проверяет открыт ли порт.
	/// @return true - если порт открыт, false - в противном случае.
	bool IsOpened() const { return m_state.CheckAll(PS_OPENED); }

	/// @brief Проверяет возможно ли чтение из порта.
	/// @return true - если чтение из порта возможно, false - в противном случае.
	bool MayRead() const { return m_state.CheckAll(PS_READ_ALLOWED); }

	/// @brief Проверяет возможна ли запись в порт.
	/// @return true - если запись в порт возможна, false - в противном случае.
	bool MayWrite() const { return m_state.CheckAll(PS_WRITE_ALLOWED); }

	/// @brief Осуществляет отправку данных через порт.
	/// @param mode Режим порта, в котором будут передаваться данные.
	/// @param ptr Указатель на массив данных для пересылки.
	/// @param len Количество байтов для пересылки.
	/// @param timeout_ms Таймаут ожидания в миллисекундах. Если значение таймаута равно INFINITE_TIMEOUT, то задача будет заблокирована без возможности разблокировки по таймауту (бесконечное ожидание).
	/// @return [ResultOk](@ref macs::ResultOk) - если операция завершена успешно, код ошибки - в противном случае.
	virtual Result Send(SendMode mode, const byte *ptr, size_t len, ulong timeout_ms = INFINITE_TIMEOUT)
		{ return SendData(mode, ptr, len, timeout_ms); }

	/// @brief Осуществляет отправку данных через порт.
	/// @param mode Режим порта, в котором будут передаваться данные.
	/// @param buf Буфер с данными для пересылки.
	/// @param timeout_ms Таймаут ожидания в миллисекундах. Если значение таймаута равно INFINITE_TIMEOUT, то задача будет заблокирована без возможности разблокировки по таймауту (бесконечное ожидание).
	/// @return [ResultOk](@ref macs::ResultOk) - если операция завершена успешно, код ошибки - в противном случае.
	Result Send(SendMode mode, const Buf &buf, ulong timeout_ms = INFINITE_TIMEOUT)
		{ return Send(mode, buf.Data(), buf.Len(), timeout_ms); }

	/// @brief Осуществляет отправку данных через порт.
	/// @param ptr Указатель на массив данных для пересылки.
	/// @param len Количество байтов для пересылки.
	/// @param timeout_ms Таймаут ожидания в миллисекундах. Если значение таймаута равно INFINITE_TIMEOUT, то задача будет заблокирована без возможности разблокировки по таймауту (бесконечное ожидание).
	/// @return [ResultOk](@ref macs::ResultOk) - если операция завершена успешно, код ошибки - в противном случае.
	Result Send(const byte *ptr, size_t len, ulong timeout_ms = INFINITE_TIMEOUT)
		{ return Send(m_def_send_mode, ptr, len, timeout_ms); }

	/// @brief Осуществляет отправку данных через порт.
	/// @param buf Буфер с данными для пересылки.
	/// @param timeout_ms Таймаут ожидания в миллисекундах. Если значение таймаута равно INFINITE_TIMEOUT, то задача будет заблокирована без возможности разблокировки по таймауту (бесконечное ожидание).
	/// @return [ResultOk](@ref macs::ResultOk) - если операция завершена успешно, код ошибки - в противном случае.
	Result Send(const Buf &buf, ulong timeout_ms = INFINITE_TIMEOUT)
		{ return Send(m_def_send_mode, buf, timeout_ms); }

	/// @brief В случае буферизованного порта, осуществляет отправку данных, находящихся в буфере.
	/// @param timeout_ms Таймаут ожидания в миллисекундах. Если значение таймаута равно INFINITE_TIMEOUT, то задача будет заблокирована без возможности разблокировки по таймауту (бесконечное ожидание).
	/// @return [ResultOk](@ref macs::ResultOk) - если операция завершена успешно, код ошибки - в противном случае.
	virtual Result Flush(ulong timeout_ms = INFINITE_TIMEOUT) { return ResultOk; }
	
	/// @brief Осуществляет прием данных через порт.
	/// @param mode Режим порта, в котором будут приниматься данные.
	/// @param buf Буфер, в который следует записать принятые данные.
	/// @param len Количество байтов для приема.
	/// @param timeout_ms Таймаут ожидания в миллисекундах. Если значение таймаута равно INFINITE_TIMEOUT, то задача будет заблокирована без возможности разблокировки по таймауту (бесконечное ожидание).
	/// @return [ResultOk](@ref macs::ResultOk) - если операция завершена успешно, код ошибки - в противном случае.
	virtual Result Receive(RecvMode mode, Buf &buf, size_t len, ulong timeout_ms)
		{ return RecvData(mode, buf, len, timeout_ms); }
	Result Receive(RecvMode mode, Buf &buf, size_t len)
		{ return RecvData(mode, buf, len, INFINITE_TIMEOUT); }  // Обход проблемы с виртуальными методами в CMLYNX

	/// @brief Осуществляет прием данных через порт.
	/// @param buf Буфер, в который следует записать принятые данные.
	/// @param len Количество байтов для приема.
	/// @param timeout_ms Таймаут ожидания в миллисекундах. Если значение таймаута равно INFINITE_TIMEOUT, то задача будет заблокирована без возможности разблокировки по таймауту (бесконечное ожидание).
	/// @return [ResultOk](@ref macs::ResultOk) - если операция завершена успешно, код ошибки - в противном случае.
	Result Receive(Buf &buf, size_t len, ulong timeout_ms = INFINITE_TIMEOUT)
		{ return Receive(m_def_recv_mode, buf, len, timeout_ms); }

	virtual bool Require(size_t len) { return true; }

protected:
	virtual Result SendData(SendMode mode, const byte *ptr, size_t len, ulong timeout_ms) = 0;
	virtual Result RecvData(RecvMode mode, Buf &buf, size_t len, ulong timeout_ms) = 0;
};


//static const size_t DEF_PORT_BUF_SIZE = Buf::DEF_BUF_SIZE;

/// @brief Шаблон порта с буферизацией, принимающего/отправляющего данные указанного типа.
template<typename buf_type>
	class BufferedPort : public Port
{
public:	
	static const size_t DEF_BUF_SIZE = 64;
	buf_type m_buffer;

public:	
	/// @brief Конструктор. Задает размер буфера [DEF_BUF_SIZE](@ref macs::DEF_BUF_SIZE).
	/// @param bufsz Размер буфера.
	BufferedPort(size_t bufsz = DEF_BUF_SIZE) { m_buffer.Alloc(bufsz); }

#ifndef __ICCARM__	
	#pragma push
	#pragma diag_suppress 997
#endif	
	/// @brief Открывает порт в указанной конфигурации с указанным размером буфера.
	/// @param config Параметры конфигурации порта.
	/// @param bufsz Размер буфера.
	/// @return true - если операция успешно выполнена, false - в противном случае.
	bool Open(const PortConfig *config = nullptr, size_t bufsz = DEF_BUF_SIZE)
	{
		bool res;
		res = Port::Open(config);		RET_ASSERT(res, false);
		res = m_buffer.Alloc(bufsz);	RET_ASSERT(res, false);
		return true;
	}

	// Обход проблемы с виртуальными методами в CMLYNX
	virtual bool ChangeState(STATE state, bool set) { return Port::ChangeState(state, set); }

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

#ifndef __ICCARM__
	#pragma pop
#endif
};

typedef BufferedPort<DefStatBuf> DefBufferedPort;

}	// namespace utils

using namespace utils;
