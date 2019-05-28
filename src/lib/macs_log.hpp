/// @file macs_log.hpp
/// @brief Журнал событий.
/// @details Предоставляет классы для работы с журналом событий.
/// @copyright AstroSoft Ltd, 2015-2016

#pragma once

#include "macs_tunes.h"

#if MACS_USE_LOG
  
#include "macs_list.hpp"
#include "macs_clock.hpp"

namespace utils {

/// @brief Базовый класс для представления различных событий.
/// @details Реализует абстрактное событие с регистрацией времени его возникновения.
class LogEvent
{
private:	
	ulong m_id;
protected:
	Time m_time;        // Время возникновения события
public:
	LogEvent * m_next;  // Указатель на следующий элемент в списке событий
public:
	/// @brief Конструктор события.
	/// @details Создаёт событие с привязкой к текущему системному времени.
	LogEvent() { m_next = nullptr; Clock::GetTime(m_time); }

	/// @brief Вывод данных события в строку.
	/// @details В базовом классе реализует вывод времени возникновения события.
	/// Данный метод может быть переопределён в классах конкретных событий для
	/// вывода дополнительных параметров.
	/// @param str - строка для вывода данных события
	/// @return Строка для вывода данных события
	virtual String & Print(String & str) const { str.Add(m_time.ToStr()); return str; } 	 
	
	friend class Log;	
};
SLIST_DECLARE(LogEventList, LogEvent, m_next);

/// @brief Журнал событий.
/// @details Реализует методы управления списком событий.
class Log
{
private:	
	ulong m_last_id;
	LogEvent * m_events;
public:
	/// @brief Конструктор журнала событий.
	/// @details Создаёт пустой журнал событий.
	Log() { m_last_id = 0; m_events = nullptr; }

	/// @brief Добавить событие в журнал.
	/// @param event - указатель на добавляемое событие.
	void Add(LogEvent * event) {
		event->m_id = ++ m_last_id;
		LogEventList::Add(m_events, event);
	}

	/// @brief Получение первой записи в журнале.
	/// @return Первая запись в журнале событий.
	const LogEvent * GetFirstEvent() { return m_events; }

	/// @brief Получение следующей записи в журнале.
	/// @param event - событие, предшествующее получаемой записи.
	/// @return Следующая запись в журнале событий.
	const LogEvent * GetNextEvent(const LogEvent * event) { return LogEventList::Next(event); }

	String & Print(String & str) const;
};
extern Log g_sys_log;	

/// @brief Системное событие.
/// @details Предназначен для регистрации различных событий системы в системном журнале.
class LogOsEvent : public LogEvent
{
public:
	/// @brief Тип системного события.
	enum KIND 
	{
		OS_STARTED 		= (0x1 << 0),  ///< Старт системы
		TASK_ADDED 		= (0x1 << 1),  ///< Добавление задачи в планировщик
		TASK_REMOVED	= (0x1 << 2)   ///< Удаление задачи из планировщика
	};
	typedef BitMask<KIND> OsEventsReg;
private:
	KIND m_kind;
	String m_task_name;
public:
	static OsEventsReg s_os_events_reg;
public:
	/// @brief Конструктор системного события.
	/// @details Создаёт системное событие указанного типа.
	/// @param kind - тип системного события
	/// @param task_name - имя задачи, с которой связано событие (опционально)
	LogOsEvent(KIND kind, CSPTR task_name = nullptr) { m_kind = kind; m_task_name.Add(task_name); }
	
	/// @brief Регистрация события в системном журнале.
	void Reg() { g_sys_log.Add(this); }

	/// @brief Вывод системного события в строку.
	/// @details Выводит в строку время и тип системного события, а также имя связанной задачи
	/// (если указано).
	/// @param str - строка для вывода данных события
	/// @return Строка для вывода данных события
	virtual String & Print(String & str) const;
};
	
}	// namespace utils 

using namespace utils;

#endif	// #if MACS_USE_LOG
