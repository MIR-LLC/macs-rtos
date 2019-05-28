#include "macs_terminal.hpp"

#if MACS_USE_TERMINAL

namespace utils {

void SubStrings::Parse(CSPTR str) 
{ 
	Clear();
	m_str.Add(str);
	SPTR p = (SPTR) (CSPTR) m_str;
	
	for (;;) {
		while ( * p == ' ' )
			++ p;
		if ( ! * p )
			return;
		
		m_arr.Add(p);
		
		while ( * p && * p != ' ' )
			++ p;
		if ( ! * p )
			return;
		
		* p ++ = '\0';
	}
}
 
#if MACS_DEBUG 
	#define KEEPALIVE_INTERVAL  (10 * 60)  // секунд
#else
	#define KEEPALIVE_INTERVAL  20  			 // секунд
#endif

const Time TermGuard::s_keepalive_interval(KEEPALIVE_INTERVAL);

CSPTR Terminal::s_endline = "\r\n";	// константа конца строки

void Terminal::Execute()
{
	StatBuf<1> buf;
	for (;;) {
		ProcessInput(buf, m_line);
		Parse();
	}
}

void Terminal::ProcessInput(Buf & buf, String & str)
{
	m_port->Receive(Port::RM_USE_SEMPH, buf, buf.Size());

	if ( m_echo )	{
		if ( String((CSPTR) buf.Data()).FindAnyChr(s_endline) == -1 )
			m_port->Send(buf);
		else
			m_port->Send((const byte *) s_endline, strlen(s_endline));
	}

	str.Add((CSPTR) buf.Data(), buf.Len());
}

// TODO сделать честную обработку терминальных escape-последовательностей
void Terminal::Parse()
{
	int pos = m_line.FindAnyChr(s_endline);		
	if ( pos == -1 )
		return;
		
	String line(m_line, pos);
	SubStrings ss(line);
	if(ss.Arr().Count()>0)	{
		String cmd(ss.Arr()[0]);
		ss.Arr().RemoveAt(0);

		if ( m_cmds.Find(cmd) )	{
			if ( m_auth_off || (! m_cmds[cmd].m_acc || m_trm_guard.DoAuthentication(m_cmds[cmd].m_acc) ) )
				m_cmds[cmd].m_cmd->DoAction(*this, ss.Arr());
			else
				WriteLine("Авторизация не выполнена!");
		} else 
			WriteLine("Команда не найдена! Введите команду help для получения помощи.");
	}
	m_line.Clear();
}
 
void Terminal::AddCommand(CSPTR name, TermCommand & cmd, byte access_lvl)
{
	if ( ! m_cmds.Find(name) )
		m_cmds.Add(TermCmdRec(name, & cmd, access_lvl));
}

// TODO правильное удаление команды
void Terminal::RemoveCommand(CSPTR name)
{
	if ( m_cmds.Find(name) )
		m_cmds.Remove(name);
}

void Terminal::WriteLine(CSPTR str, bool end_line)
{
	if ( ! m_started )
		return;

	m_port->Send((const byte *) str, strlen(str));
    if( end_line )
        m_port->Send((const byte *) s_endline, strlen(s_endline));
}

void Terminal::ReadLine(String & str)
{
	str.Clear();
		
	if ( ! m_started )
		return;

	StatBuf<1> buf;
	String line;

	for (;;) {
		ProcessInput(buf, line);

		int pos = line.FindAnyChr(s_endline);
		if ( pos != -1 ) {
			str.Add(line, pos);
			break;
		}
	}
}

// TODO уточнить TaskMode и TaskPriority
Result Terminal::Start()
{
	if (!m_port || !m_port->IsOpened()) 
		return ResultErrorInvalidState;

	Result res = Task::Add(this, Task::PriorityNormal, Task::ModePrivileged);
	if (res == ResultOk)
		m_started = true;

	return res;
}

bool TermGuard::Login()
{
	m_term.WriteLine("Требуется авторизация!");
	m_term.WriteLine("Имя пользователя:");
	String username; m_term.ReadLine(username);

	m_term.WriteLine("Пароль (символы не выводятся):");
	m_term.SetEchoMode(false);
	String passwd; m_term.ReadLine(passwd);
	m_term.SetEchoMode(true);

	uint32_t pwd_crc32 = g_crc32.Calc((const byte *) (CSPTR) passwd, passwd.Len());

	int ind = m_users.IndexOf((CSPTR) username);
	if ( ind == DynArr<TermUser>::NPOS || m_users[ind].m_password != pwd_crc32 )
		return false;
	 
	m_curr_usr = & m_users[ind];
	return true;
}

bool TermGuard::ValidatePrivilege(byte access_lvl)
{
	return m_curr_usr->m_access_level >= access_lvl;
}
	
// TODO расширенный статус ошибки
// TODO запись в журнал попыток входа и их статусов
// TODO возможность смены пользователя в активной сессии (если не достаточно привилегий для выполнения команды)
bool TermGuard::DoAuthentication(byte access_lvl)
{
	bool timeout = (Clock::GetTime() - m_time > s_keepalive_interval);
	if ( timeout ) 
		m_term.WriteLine("Время сессии вышло!");

	if ( m_time.IsZero() || timeout || ! ValidatePrivilege(access_lvl) ) 
		if ( ! Login() )
			return false;

	m_time = Clock::GetTime();
	return ValidatePrivilege(access_lvl);
}
 
}	// namespace utils
 
#endif	// #if MACS_USE_TERMINAL
