/// @file macs_profiler.hpp
/// @brief Профилировщик.
/// @details Предоставляет интерфейс к профилировщику, способному точно измерять временные характеристики приложения.
/// Профилировщик учитывает собственную задержку, так что ее величина не на резальтаты измерений.
/// Для использования профилировщика необходимо включить опцию MACS_PROFILING_ENABLED в настройках системы.
/// @copyright AstroSoft Ltd, 2016
 
#pragma once
 
#include "macs_tunes.h"

#if MACS_PROFILING_ENABLED 

#include <math.h>

#include "macs_common.hpp"

/// @brief Пространство имён для инструментов производительности.
namespace performance {

typedef enum 
{	// Системный раздел профилировщика - не использовать!
	PE_EMPTY_CALL = 0,	
	PE_EMPTY_CONSTR,	
	PE_EMBRACE,	
	
	// Тестовый раздел - не использовать!
	PE_ZERO_CALL_A1,	
	PE_ZERO_CALL_B1,	
	PE_ZERO_CONSTR_A1,	
	PE_ZERO_CONSTR_B1,	
	PE_ZERO_CONSTR_B1_2,	
	PE_ZERO_CONSTR_C1,	
	PE_ZERO_CONSTR_C1_2,	
	PE_ZERO_CONSTR_D1_2,	
	PE_ZERO_CONSTR_D1_2_3,	
	
	PE_INCR_INT,
	
	// Раздел операционной системы - не использовать!
	PE_CRIT_SEC_INT_ENTR,
	PE_CRIT_SEC_INT_EXIT,
	PE_CRIT_SEC_EXT_ENTR,
	PE_CRIT_SEC_EXT_EXIT,
	
	PE_MEM_ALLOC,
	PE_MEM_FREE,
	
	PE_TASK_INIT,
	PE_TASK_ADD,
	PE_TASK_DEL,
	
	PE_IQR_HANDLE,
	
	PE_EVENT_INIT,
	PE_EVENT_RAISE,
	PE_EVENT_ACTION,
	
	PE_MUTEX_INIT,
	PE_MUTEX_LOCK,
	PE_MUTEX_UNLOCK,
	PE_MUTEX_ACTION,
	
	PE_SEMPH_INIT,
	PE_SEMPH_GIVE,
	PE_SEMPH_TAKE,
	PE_SEMPH_ACTION,
	
	PE_DELAY_10MS,
	 
	// Поместите ваши разделы начиная с этого места...
	
	PE_USER_1,
	PE_USER_2,
	PE_USER_3,
	 
	PE_QTTY
} PROF_EYE;

/// @brief Объект профилировщика.
/// @details Объект профилировщика, предназначенный для измерения временных характеристик. 
/// @details Внимание! Рекомендуется пользоваться не методами класса, а макросами, определенными ниже.
class ProfEye
{
public:	
	/// @brief Создает объект профилировщика. 
	/// @details Внимание! Рекомендуется пользоваться не методами класса, а макросами, определенными ниже.
	/// @param eye Идентификатор раздела.
	/// @param run Если истина, то отсчет времени начинается немедленно.
	ProfEye(PROF_EYE eye, bool run);	

 ~ProfEye();
	
	inline PROF_EYE Eye() const { return m_eye; }			 

	/// @brief Начинает отсчет времени. 
	/// @details Внимание! Рекомендуется пользоваться не методами класса, а макросами, определенными ниже.
	void Start();

	/// @brief Завершает отсчет времени. 
	/// @details Внимание! Рекомендуется пользоваться не методами класса, а макросами, определенными ниже.
	/// @param call Истина - вызов сделан непосредственно, ложь - из деструктора.
	void Stop(bool call = true);

	/// @brief Сбрасывает отсчет времени. 
	/// @details Сбрасывает отсчет времени. Последний запуск игнорируется.
	void Kill() { m_run = false; }
	
	// Системный метод. Вызывается ОС для калибровки в начале работы. 
	static void Tune();
	
	/// @brief Печать статистики. 
	/// @details Выводит накопленную статистику в строку.
	void Print(String & str, bool brief = false, bool use_ns = false);
	
	/// @brief Печать всей статистики. 
	/// @details Выводит всю накопленную статистику в строку.
	static void PrintResults(String & str, bool brief = false, bool use_ns = false);
	
private:
	bool m_run;
	PROF_EYE m_eye;			
	tick_t m_start;
	long m_lost;
	ProfEye * m_up_eye;
	static ProfEye * s_cur_eye;
};

class ProfData
{
private:
	bool m_lock;
	long m_time;	// Может быть отрицательным для очень коротких промежутков :)
	long m_lost;	
	uint64_t m_sqrs;
	long m_min, m_max;
	ulong  m_cnt;

	static tick_t s_empty_call_overhead;
	static tick_t s_empty_constr_overhead;
	static tick_t s_embrace_overhead;
	static const int ADJUSTMENT =	19;
public:
	ProfData() { m_lock = false; Clear(); }
	
	inline void Clear() { m_lost = m_max = m_time = 0; m_sqrs = 0; m_min = LONG_MAX; m_cnt = 0; }
	
	/// @brief Количество вызовов. 
	/// @details Возвращает количество вызовов данного участка кода.
	inline ulong Count() const { return m_cnt; }
	
	/// @brief Полное время. 
	/// @details Возвращает полное время, затраченное на выполнение данного участка кода.
	inline long  TimeTot() const { return m_time + m_lost; }
	
	/// @brief Чистое время. 
	/// @details Возвращает время, затраченное на выполнение данного участка кода, за вычетом времени выполнения вложенных участков.
	inline long  TimeNet() const { return m_time; }
	
	/// @brief Чужое время. 
	/// @details Возвращает время, затраченное на выполнение вложенных участков кода.
	inline long  TimeOvh() const { return m_lost; }
	
	/// @brief Среднее время. 
	/// @details Возвращает среднее чистое время, затраченное на выполнение участка кода.
	inline long  TimeAvg() const { return m_cnt ? TimeNet() / m_cnt : 0; }
	
	/// @brief Минимальное время. 
	/// @details Возвращает минимальное чистое время, затраченное на выполнение участка кода.
	inline long  TimeMin() const { return m_cnt ? m_min : 0; }
	
	/// @brief Максимальное время. 
	/// @details Возвращает максимальное чистое время, затраченное на выполнение участка кода.
	inline long  TimeMax() const { return m_cnt ? m_max : 0; }
	
	/// @brief Среднеквадратичное отклонение. 
	/// @details Возвращает среднеквадратичное отклонение чистого время, затраченного на выполнение участка кода.
	inline ulong TimeDev() const { return m_cnt ? sqrt((double) (m_sqrs / m_cnt - TimeAvg() * (int64_t) TimeAvg())) : 0; }
	
	/// @brief Печать статистики. 
	/// @details Выводит накопленную статистику в строку.
	void Print(String & str, bool brief = false, bool use_ns = false);
	 
private:
	void 	Lock(bool set) { _ASSERT(set != m_lock); m_lock = set; }
	
	friend class ProfEye;
};
extern ProfData	g_prof_data[PE_QTTY];

}	// namespace performance 
  
using namespace performance;
	
	/// @brief Создает объект и запускает отсчет времени. 
	/// @details Измеряется время существования объекта. Объект уничтожается при выходе из области видимости.
	/// @param eye Идентификатор раздела.
	/// @param name Любое допустимое имя объекта.
	#define PROF_EYE(eye, name) 	ProfEye pe_##name(eye, true)
		
	/// @brief Создает объект, но НЕ запускает отсчет времени. 
	/// @details Запуск и остановка измерения делаются вручную. 
	/// @param eye Идентификатор раздела.
	/// @param name Любое допустимое имя объекта.
	#define PROF_DECL(eye, name) 	ProfEye name(eye, false);

	/// @brief Запускает отсчет времени. 
	/// @details Предыдущий запуск (если был) учитывается независимо. 
	/// @param name Имя объекта.
	#define PROF_START(name) 			name.Start();

	/// @brief Останавливает отсчет времени. 
	/// @details Результат добавляется к статистике. Новый запуск (если будет) учитывается независимо.
	/// @param name Имя объекта.
	#define PROF_STOP(name) 			name.Stop();
	
#else
	
	/// @brief Заглушка для упрощения кодирования. 
	/// @details Если профилировщик выключен, то обращения к нему заменяются на пустые операторы.
	/// Для использования профилировщика необходимо включить опцию MACS_PROFILING_ENABLED в настройках системы.
	#define PROF_EYE(eye, name)

	/// @brief Заглушка для упрощения кодирования. 
	/// @details Если профилировщик выключен, то обращения к нему заменяются на пустые операторы.
	/// Для использования профилировщика необходимо включить опцию MACS_PROFILING_ENABLED в настройках системы.
	#define PROF_DECL(eye, name)

	/// @brief Заглушка для упрощения кодирования. 
	/// @details Если профилировщик выключен, то обращения к нему заменяются на пустые операторы.
	/// Для использования профилировщика необходимо включить опцию MACS_PROFILING_ENABLED в настройках системы.
	#define PROF_START(name)

	/// @brief Заглушка для упрощения кодирования. 
	/// @details Если профилировщик выключен, то обращения к нему заменяются на пустые операторы.
	/// Для использования профилировщика необходимо включить опцию MACS_PROFILING_ENABLED в настройках системы.
	#define PROF_STOP(name)
	
#endif	// #if MACS_PROFILING_ENABLED 
