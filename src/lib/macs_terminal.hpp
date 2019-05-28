/// @file macs_terminal.hpp
/// @brief Служба терминала.
/// @details Реализация службы терминала. Поддерживает работу пользовательских команд.
/// @copyright AstroSoft Ltd, 2015-2016

#pragma once

#include "macs_tunes.h"

#if MACS_USE_TERMINAL

#include "macs_common.hpp"
#include "macs_port.hpp"
#include "macs_clock.hpp"

namespace utils {
	
class SubStrings
{
private:
	String 			m_str;
	DynArr<CSPTR> m_arr;
public:
	SubStrings(CSPTR str) { if ( str ) Parse(str); }
	DynArr<CSPTR> & Arr() { return m_arr; }
	void Clear() { m_str.Clear(); m_arr.Clear(); }
	void Parse(CSPTR str);
};


class Terminal;

/// @brief Интерфейс пользовательской команды.
/// @details Пользовательская команда, добавляемая в терминал
/// и вызываемая при вводе её псевдонима, устанавливаемого при добавлении.
class TermCommand
{
public:
	CSPTR m_comment;
public:
	TermCommand(CSPTR comment = nullptr) : m_comment(comment) {}
	/// @brief Выполняет команду.
	/// @param term Ссылка на экземпляр службы терминала, в котором вызвалась команда.
	/// @param args Контейнер с аргументами, переданными команде.
	virtual void DoAction(Terminal & term, const DynArr<CSPTR> & args) = 0;
};
 
class TermCmdRec
{
public:
	CSPTR 				m_name;
	TermCommand * m_cmd;
	byte 					m_acc;
public:
	TermCmdRec(CSPTR name = nullptr, TermCommand * cmd = nullptr, byte acc = 0) : m_name(name) { m_cmd = cmd; m_acc = acc; }
};
inline bool operator == (const TermCmdRec & c1, const TermCmdRec & c2) { return strcmp(c1.m_name, c2.m_name) == 0; }

typedef DynArr<TermCmdRec> TermCmdRecArr;

class TermCommands : public TermCmdRecArr
{
public:
	inline bool Find(CSPTR name) { return IndexOf(name) != NPOS; }
	inline TermCmdRec & operator [] (CSPTR name) { return (* this)[IndexOf(name)]; }
	inline TermCmdRec & operator [] (uint ind) { return TermCmdRecArr::operator [] (ind); }
	inline const TermCmdRec & operator [] (uint ind) const { return TermCmdRecArr::operator [] (ind); }
};

/// @brief Пользователь.
/// @details Содержит данные пользователя для контроля привилегий.
struct TermUser
{
	/// @brief Имя пользователя.
	String m_username;

	/// @brief Пароль пользователя.
	uint32_t m_password;

	/// @brief Уровень привилегий пользователя.
	/// @details Наименьший уровень - 0, наибольший - 255.
	byte m_access_level;

	/// @brief Конструктор.
	/// @param username Имя пользователя.
	/// @param password Пароль пользователя (CRC32).
	/// @param access_level Уровень привилегий пользователя.
	TermUser(CSPTR username = "", uint32_t password = 0, byte access_level = 0) :
		m_username(username),
		m_password(password),
		m_access_level(access_level)
	{}

	bool operator == (const TermUser & user) {
		return m_username == user.m_username;
	}
};


/// @brief Страж терминала.
/// @details Производит контроль привилегий пользователей.
class TermGuard
{
private:
	Terminal & m_term;
	Time m_time;
	DynArr<TermUser> m_users;
	TermUser * m_curr_usr;

	static const Time s_keepalive_interval; // время активности сессии

private:
	bool Login();
	bool ValidatePrivilege(byte access_lvl);

public:
	/// @brief Конструктор. Создаёт стража, работающего с указанным терминалом.
	/// @details Также в конструкторе создаются и добавляются пользователи.
	/// @param terminal Терминал.
	TermGuard(Terminal & terminal) :
		m_term(terminal)
	{
		m_curr_usr = nullptr;
		m_users.Add(TermUser("guest", 0xACB79A35, 1));
		m_users.Add(TermUser("root", 	0x16F4F95B, 255));
	}

	/// @brief Производит аутентификацию пользователя для выполнения команды.
	/// @details Успешная аутентификация создаёт сессию, действительную устанавливаемое количество секунд.
	/// Во время действительной сессии не требуется повторно вводить логин/пароль пользователя.
	/// @param access_lvl Запрашиваемый необходимый уровень привилегий.
	/// @return Результат аутентификации: true - успешно, false - неуспешно.
	bool DoAuthentication(byte access_lvl);
};


/// @brief Служба терминала.
/// @details Терминал работает через порт (например, PortUart).
class Terminal : public Task
{
private:
	Port * m_port;
    bool m_auth_off;
	String m_line;
	TermCommands m_cmds;
	bool m_started;
	bool m_echo;
	TermGuard m_trm_guard;

	static CSPTR s_endline;	// константа конца строки

private:
	virtual void Execute();
	void ProcessInput(Buf & buf, String & str);
	void Parse();

public:
	/// @brief Конструктор. Создаёт службу терминала, работающую через указанный порт.
	/// @param port Порт.
	Terminal(Port * port = nullptr, bool auth_off = false ) :
		Task("Terminal"),
		m_port(port),
        m_auth_off(auth_off),
		m_started(false),
		m_echo(true),
		m_trm_guard(*this)
	{}
		 
	/// @brief Устанавливает порт для терминала.
	/// @param port Порт.
	void SetPort(Port & port) { m_port = & port; }

	/// @brief Устанавливает режим эха для терминала.
	/// @param mode Порт.
	void SetEchoMode(bool mode) { m_echo = mode; }

	/// @brief Возвращает статус режима эхо терминала.
	bool IsEchoEnabled() const { return m_echo; }

	/// @brief Добавляет команду в службу терминала и связывает её с указанным псевдонимом.
	/// @param name Псевдоним команды.
	/// @param cmd Команда.
	/// @param access_lvl Уровень доступа, необходимый для команды.
	void AddCommand(CSPTR name, TermCommand & cmd, byte access_lvl = 1);

	/// @brief Удаляет команду из службы терминала.
	/// @param name Псевдоним команды.
	void RemoveCommand(CSPTR name);

	const TermCommands & Commands() const { return m_cmds; }
	
	/// @brief Записывает строку в порт, с которым работает служба терминала.
	/// @param str Строка для записи в порт.
	void WriteLine(CSPTR str,bool end_line = true);

	/// @brief Считывает строку из порта, с которым работает служба терминала.
	/// @details Этот метод возвращает строку только после её ввода в терминал.
	/// @return Строка, считанная из порта.
	void ReadLine(String & str);

	/// @brief Запускает службу терминала.
	/// @details К моменту вызова этого метода порт должен быть настроен и открыт.
	Result Start();
};

} //  namespace utils
using namespace utils;
 
#endif	// #if MACS_USE_TERMINAL
