/// @file macs_clock.hpp
/// @brief Метки времени.
/// @details Предоставляет классы для работы с системными часами.
/// @copyright AstroSoft Ltd, 2015-2016

#pragma once

#include "macs_tunes.h"

#if MACS_USE_CLOCK

#include "macs_system.hpp"
#include "macs_common.hpp"

namespace macs {
class Scheduler;
}
 
namespace utils {

/// @brief Базовый класс для представления меток времени.
/// @details Хранит относительное значение времени, прошедшее с определённого момента.
/// Для объектов данного класса доступны операторы:
///  - сравнение меток времени (==, <, >)
///  - увеличение метки времени (+=)
class Time
{
private:	
	static char s_prn_buf[];

public:
	/// @brief Секундная составляющая значения времени.
	/// @details Хранит количество секунд, прощедшее с определённого момента времени.
	uint32_t m_scnd;

	/// @brief Аппаратная составляющая значения времени.
	/// @details Хранит количество тактов процессора, добавляемое к секундной
	/// составляющей для повышения точности значения времени.
	/// Масштаб времени этого значения зависит от системной частоты.
	uint32_t m_frac;

public:
	/// @brief Конструктор.
	/// @details Пустой конструктор.
	Time() {}

	/// @brief Конструктор.
	/// @details Конструктор, инициализирующий метку времени указанными параметрами.
	/// @param sec Секунд.
	/// @param min Минуты.
	/// @param hrs Часы.
	/// @param days Дней.
	Time(uint32_t sec, uint32_t min = 0, uint32_t hrs = 0, uint32_t days = 0)
	{
		m_scnd = sec + min * 60 + hrs * 3600 + days * 86400;
		m_frac = 0;
	}

	/// @brief Обнуление значения времени.
	inline void Zero() { m_scnd = m_frac = 0; }

	/// @brief Проверка, обнулено ли значение времени.
	/// @return true - значение времени обнулено, false - в противном случае
	inline bool IsZero() const { return m_scnd == 0 && m_frac == 0; }

	/// @brief Проверка, является ли значение времени нормализованным.
	/// @details Значение времени считается нормализованным, если его аппаратная
	/// составляющая не превышает значение системной частоты.
	/// @return true - значение времени является нормализованным, false - в противном случае
	inline bool IsNorm() const { return m_frac < System::GetCpuFreq(); }

	/// @brief Нормализация значения времени.
	/// @details Выполняет пересчёт ненормализованного значения времени следующим образом:
	///  - секундная составляющая увеличивается на количество полных секунд, содержащихся
	///    в аппаратной составляющей (результат деления на значение системной частоты)
	///  - аппаратная составляющая уменьшается по модулю значения системной частоты.
	inline void Norm() {
		if ( ! IsNorm() ) {
			m_scnd += m_frac / System::GetCpuFreq();
			m_frac %= System::GetCpuFreq();
		}
	}

	/// @brief Получение количества миллисекунд из метки времени.
	/// @return Количество миллисекунд
	inline uint GetMs() const { 
		_ASSERT(IsNorm());
		return m_frac / (System::GetCpuFreq() / 1000);
	}

	/// @brief Получение количества секунд из метки времени.
	/// @return Количество секунд
	inline uint GetS() const {
		_ASSERT(IsNorm());
		return m_scnd % 60;
	}

	/// @brief Получение количества минут из метки времени.
	/// @return Количество минут
	inline uint GetM() const {
		_ASSERT(IsNorm());
		return (m_scnd / 60) % 60;
	}

	/// @brief Получение количества часов из метки времени.
	/// @return Количество часов
	inline uint GetH() const {
		_ASSERT(IsNorm());
		return (m_scnd / (60 * 60)) % 24;
	}

	/// @brief Получение количества дней из метки времени.
	/// @return Количество дней
	inline uint GetD() const {
		_ASSERT(IsNorm());
		return m_scnd / (24 * 60 * 60);
	}

	/// @brief Строковое представление значения времени.
	/// @details Возвращает указатель на строку со значением метки времени.
	/// @param verb - формат вывода значения времени:
	///  - false - используется короткое представление "00m00s.000"
	///  - true - используется длинное представление "000d00h00m00s.000"
	/// @return Строка со значением метки времени
	CSPTR ToStr(bool verb = false) const;
};	 


inline bool operator == (const Time & t1, const Time & t2) 
{
	_ASSERT(t1.IsNorm());
	_ASSERT(t2.IsNorm());
	return t1.m_scnd == t2.m_scnd && t1.m_frac == t2.m_frac;
}

inline bool operator > (const Time & t1, const Time & t2) 
{
	_ASSERT(t1.IsNorm());
	_ASSERT(t2.IsNorm());
	return t1.m_scnd != t2.m_scnd ? (t1.m_scnd > t2.m_scnd) : (t1.m_frac > t2.m_frac);
}

inline bool operator < (const Time & t1, const Time & t2) 
{
	_ASSERT(t1.IsNorm());
	_ASSERT(t2.IsNorm());
	return t1.m_scnd != t2.m_scnd ? (t1.m_scnd < t2.m_scnd) : (t1.m_frac < t2.m_frac);
}

inline Time operator + (const Time & t1, const Time & t2)
{
	_ASSERT(t1.IsNorm());
	_ASSERT(t2.IsNorm());
	Time t;
	t.m_scnd = t1.m_scnd + t2.m_scnd;
	t.m_frac = t1.m_frac + t2.m_frac;
	t.Norm();
	return t;
}

// TODO разобраться с возможными переполнения при вычитании
inline Time operator - (Time t1, const Time & t2)
{
	_ASSERT(t1.IsNorm());
	_ASSERT(t2.IsNorm());
	if (t1.m_frac < t2.m_frac)
	{
		t1.m_frac += System::GetCpuFreq();
		t1.m_scnd -= 1;
	}
	t1.m_scnd -= t2.m_scnd;
	t1.m_frac -= t2.m_frac;
	return t1;
}

inline Time & operator += (Time & t1, const Time & t2) 
{
	_ASSERT(t1.IsNorm());
	_ASSERT(t2.IsNorm());
	t1.m_scnd += t2.m_scnd;
	t1.m_frac += t2.m_frac;
	t1.Norm();
	return t1;
}
 
// TODO разобраться с возможными переполнения при вычитании
inline Time & operator -= (Time & t1, const Time & t2) 
{
	_ASSERT(t1.IsNorm());
	_ASSERT(t2.IsNorm());
	if (t1.m_frac < t2.m_frac)
	{
		t1.m_frac += System::GetCpuFreq();
		t1.m_scnd -= 1;
	}
	t1.m_scnd -= t2.m_scnd;
	t1.m_frac -= t2.m_frac;
	return t1;
}

////////////////////////////////////////////////////////////////

/// @brief Системные часы.
/// @details Реализует интерфейс получения системных меток времени.
class Clock
{
private:
	static uint32_t s_cur_scnd;
	static uint32_t s_last_frac;	
	static uint32_t s_last_scnd_tick;

public:
	/// @brief Получение метки времени относительно старта системы.
	/// @param time - переменная, в которую будет помещено текущее значение системного времени.
	static void GetTime(Time & time);

	/// @brief Получение метки времени относительно старта системы.
	/// @return Текущее значение системного [времени](@ref utils::Time)
	static inline Time GetTime() {	// Медленный, но удобный вариант
		Time time;
		GetTime(time);
		return time;
	}

private:
	static void OnTick(uint32_t tick);		// Вызывается ядром из критической секции или паузы планировщика.

	friend class macs::Scheduler;
};

}	// namespace utils 

using namespace utils;

#endif	// #if MACS_USE_CLOCK
