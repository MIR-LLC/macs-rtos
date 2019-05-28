/// @file macs_common.hpp
/// @brief Общие определения для кода ОС 
/// @copyright AstroSoft Ltd, 2016
 
#pragma once

#include <stdint.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

#include "macs_tunes.h"
#include "macs_nullptr.h"
#include "macs_utils.hpp"
//!!#include "kernel_bkpt.h"
#include "compiler.h"
   
#define MACS_VERSION_CODE(v1, v2, v3, v4) ( (v1) << 3*8 | (v2) << 2*8 | (v3) << 1*8 | (v4) << 0*8 );	

/// Пространство имен ядра ОС
namespace macs { 
	       
/// @brief Версия ОС.
/// @details Четыре байта определяют (1 - старший):
/// 1. Глобальный номер версии, увеличивается после введения принципиальных изменений.
/// 2. Увеличивается после изменений, способных повлиять на поведение системы.
/// 3. Увеличивается после мелкого изменения или исправления ошибки.
/// 4. Увеличивается после любого	изменения кода.
const int Version = MACS_VERSION_CODE(1, 6, 0, 0);   
   
// Используется чтобы запретить методы копирования класса.
#define CLS_COPY(cls_name) cls_name(const cls_name &); cls_name & operator = (const cls_name &);

// Индексы методов, которые допустимо вызывать через SVC 1 (с привилегированным доступом)
enum EPrivilegedMethods
{
	EPM_Read_Cpu_Tick,
	EPM_BlockCurrentTask_Priv,
	EPM_AddTask_Priv,
	EPM_AddTaskIrq_Priv,
	EPM_Yield_Priv,
	EPM_DeleteTask_Priv,
	EPM_UnblockTask_Priv,
	EPM_SetTaskPriority_Priv,
	EPM_Event_Raise_Priv,
	EPM_Event_Wait_Priv,
	EPM_Mutex_Lock_Priv, 
	EPM_Mutex_Unlock_Priv,
	EPM_Semaphore_Wait_Priv,
	EPM_Semaphore_Signal_Priv,
	EPM_SpiTransferCore_Initialize_Priv,
	EPM_Spi_PowerControl_Priv,

	EPM_Count		// последней элемент - индекс метода должен быть 0 <= index < EMP_Count
};
 
#if MACS_DEBUG
extern void _Assert(bool);
	#define _ASSERT(e) _Assert(e)
#else
	#define _ASSERT(e) 
#endif

#define RET_ERROR(res, ret) { if ( ! (res) ) return (ret); }
#define RET_ASSERT(res, ret) { _ASSERT(res); RET_ERROR(res, ret); }

/// @brief Причины исключительных ситуаций системы
typedef enum
{
	AR_NONE = 0,
	
	AR_NMI_RAISED,            ///< Произошло немаскируемое прерывание (Non Maskable Interrupt, NMI)
	AR_HARD_FAULT,            ///< Аппаратная проблема (произошло прерывание Hard Fault)
	AR_MEMORY_FAULT,          ///< Произошла ошибка доступа к памяти (MemManage interrupt)
	AR_NOT_IN_PRIVILEGED,     ///< Произошла попытка выполнить привилегированную операцию в непривилегированном режиме
	AR_BAD_SVC_NUMBER,        ///< Произошла попытка использовать SVC с некорректным номером сервиса
	AR_COUNTER_OVERFLOW,      ///< Произошло переполнение счетчика 
	AR_STACK_CORRUPTED,       ///< Затерт маркер на верхней границе стека
	AR_STACK_OVERFLOW,        ///< Произошло переполнение стека задачи     
	AR_STACK_UNDERFLOW,       ///< Произошел выход за нижнюю границу стека 
	AR_SCHED_NOT_ON_PAUSE,    ///< Произошла попытка продолжить выполнение планировщика не в состоянии паузы
	AR_MEM_LOCKED,            ///< Менеджер памяти заблокирован
	
	AR_USER_REQUEST,          ///< Вызвано пользователем

	AR_ASSERT_FAILED,         ///< Условие проверки ASSERT не выполнилось
	
	AR_STACK_ENLARGED,        ///< Возникла необходимость в увеличении стека задачи
	AR_OUT_OF_MEMORY,         ///< Память "кучи" исчерпана
	AR_SPRINTF_TRUNC,         ///< Вывод функции sprintf был урезан из-за нехватки места в буфере
	AR_DOUBLE_PRN_FMT,        ///< Более одного последовательного вызова PrnFmt в одной и той же задаче
	AR_NESTED_MUTEX_LOCK,     ///< Произошла попытка повторно заблокировать нерекурсивный мьютекс одной и той же задачей
	AR_OWNED_MUTEX_DESTR,     ///< Мьютекс, захваченный одной из задач, удалён	
	AR_BLOCKING_MUTEX_DESTR,  ///< Мьютекс, блокировавший одну или несколько задач, удалён	
    AR_PRIV_TASK_ADDING,
	AR_NO_GRAPH_GUARD,				///< Попытка выполнить операцию рисования без использования GraphGuard
	 
	AR_UNKNOWN                ///< Неизвестная ошибка
} ALARM_REASON;

/// @brief Действия при возникновении исключительной ситуации
typedef enum 
{
	AA_CONTINUE,       ///< Продолжить выполнение задачи
	AA_RESTART_TASK,   ///< Перезапустить вызвавшую ошибку задачу
	AA_KILL_TASK,      ///< Снять с выполнения задачу, вызвавшую ошибку
	AA_CRASH           ///< Останов системы
} ALARM_ACTION;

extern uint8_t ExclSet(uint8_t & flag);          // Возвращает предыдущее значение флага и устанавливает новое
extern uint8_t ExclIncCnt(uint8_t & cnt);        // Возвращает предыдущее значение счётчика, увеличивает счётчик на 1
extern ulong ExclIncCnt(ulong & cnt);      // Возвращает предыдущее значение счётчика, увеличивает счётчик на 1
extern long ExclChg(long & val, long chg); // 
extern void * ExclSetPtr(void * & ptr, void * new_val);	// Возвращает предыдущее значение указателя и устанавливает новое

// Эти функции могут быть использованы без обращения к объектам ОС (например, для вызова из C-кода)
typedef uint32_t tick_t; 
extern "C" tick_t MacsGetTickCount();       // Аналогично вызову Scheduler::GetInstance().GetTickCount()
extern "C" void MacsDelay(ulong ticks);     // Аналогично вызову Task::Delay()
extern "C" void MacsCpuDelay(ulong ticks);  // Аналогично вызову Task::CpuDelay()

template<typename T>
	inline void Swap(T & v1, T & v2) { T tmp = v1; v1 = v2; v2 = tmp; }

class LazyBoy
{
private:	
	long m_start;
public:
	LazyBoy(bool mark = true) { if ( mark ) Mark(); else m_start = 0; }
	void Mark() { m_start =  MacsGetTickCount(); }
	long Spend() { return MacsGetTickCount() - m_start; }
	void Rest(long ticks, bool task_delay = true, bool min_one = false) {
		ticks -= Spend();
		if ( ticks > 0 || (ticks == 0 && min_one) ) 
			task_delay ? MacsDelay(ticks) : MacsCpuDelay(ticks); 
	}
};

class SetFlagTemp 
{
private:	
	bool & m_flag;
	bool m_save;
public:
	SetFlagTemp(bool & flag, bool val) : m_flag(flag) { m_save = flag; flag = val; }
 ~SetFlagTemp() { m_flag = m_save; }
};
#define SET_FLAG_TEMP(flag, val) SetFlagTemp sft_##flag(flag, val)

template <typename bm_enum>
	class BitMask	
{
private:	
	bm_enum m_val;
public:
	BitMask() { Zero(); }
	BitMask(bm_enum val) { m_val = val; }
	bm_enum Val() const { return m_val; }
	void Zero() { m_val = (bm_enum) 0; } 
	bool Check(bm_enum val) const { return CheckAll(val); }
	bool CheckAny(int val) const { return m_val & val; }
	bool CheckAll(int val) const { return (m_val & val) == val; }	
	void Set(int val) { m_val = (bm_enum) val; }
	void Add(int val) { m_val = (bm_enum) (m_val | val); }
	void Rem(int val) { m_val = (bm_enum) (((uint) m_val) & (~ (uint) val)); }
};

typedef uint BitArrCell;
class BitArr
{
private:
	BitArrCell * m_arr;
protected:
	uint m_qty;
public:
	BitArr() { Init(); }
	BitArr(uint qty) { Init(); Alloc(qty); }
 ~BitArr() { Free(); }
	void Alloc(uint qty) { Free(); m_qty = qty; m_arr = new BitArrCell[Cells(qty)]; memset(m_arr, '\0', Cells(qty)); }
	bool Check(uint ind) { 
		_ASSERT(ind < m_qty); 
		return !! (m_arr[ind / BitsPerCell()] & CellMask(ind)); 
	}
	void Set(uint ind, bool val) { 
		_ASSERT(ind < m_qty); 
		val ? m_arr[ind / BitsPerCell()] |= CellMask(ind) :
					m_arr[ind / BitsPerCell()] &= ~ CellMask(ind);
	}
private:
	CLS_COPY(BitArr)
	void Init() { m_qty = 0; m_arr = nullptr; }
	void Free() { if ( m_arr ) delete[] m_arr; Init(); }
	static uint BitsPerCell() { return 8 * sizeof(BitArrCell); }
	static uint Cells(uint qty) { return (qty + (BitsPerCell() - 1)) / BitsPerCell(); }
	static BitArrCell CellMask(uint ind) { return 1 << (ind % BitsPerCell()); }
};

class BitArr2 :
	public BitArr
{
private:
	uint m_width;
public:
	BitArr2() { m_width = 0; }
	BitArr2(uint x_size, uint y_size) { Alloc(x_size, y_size); }	
	void Alloc(uint x_size, uint y_size) { BitArr::Alloc(x_size * y_size); m_width = x_size; }		
	bool Check(uint x_ind, uint y_ind) { 
		_ASSERT(x_ind < m_width); 
		return BitArr::Check(BitInd(x_ind, y_ind)); 
	}	
	void Set(uint x_ind, uint y_ind, bool val) { 
		_ASSERT(x_ind < m_width); 
		BitArr::Set(BitInd(x_ind, y_ind), val);
	}
private:
	CLS_COPY(BitArr2)
	uint BitInd(uint x_ind, uint y_ind) { return x_ind * m_width + y_ind; }
};
 
class SpinLock
{
private:
	volatile uint32_t m_lockVar;
public:
	SpinLock() { m_lockVar = 0; }
	void Lock();
	void Unlock();
};

class LockGuard
{
private:
	SpinLock & m_lock;
public:
	explicit LockGuard(SpinLock & lock) : m_lock(lock) { m_lock.Lock(); }
 ~LockGuard() { m_lock.Unlock(); }
private:
	CLS_COPY(LockGuard)
};

extern int RandN(int n); // [1..n]
inline int RandMM(int min_val, int max_val) { return min_val + (RandN((max_val - min_val) + 1) - 1); }	// [min_val..max_val]
inline bool RandCoin() { return RandN(2) == 1; }

class PrnFmt
{
private:	
	static const int SPRINTF_BUFSZ = 128;
	static char * s_buf;
public:
	PrnFmt(CSPTR format, ...);
 ~PrnFmt();
	operator CSPTR () { return s_buf; }
};
extern void Sprintf(char * buf, size_t bufsz, CSPTR format, ...);
 
extern CSPTR const g_zstr; 
inline CSPTR ZSTR(CSPTR str) { return str ? str : g_zstr; }
class String
{
public:	
	static CSPTR const NEWLINE; 
private:	
	char * m_str;
public:	
	String(CSPTR str = nullptr, int len = -1) { 
		m_str = nullptr; 
		if ( str ) {
			if ( len == -1 )
				len = strlen(str);
			else
				_ASSERT(len >= 0 && len <= strlen(str));
			Add(str, len); 
		}
	}
 ~String() { Clear(); }
	String & operator = (const String & str) { return (* this) = (CSPTR) str; }
	String & operator = (CSPTR str) { Clear(); Add(str); return * this; }
	bool operator ! () const { return ! m_str; }
	inline size_t Len() const { return m_str ? strlen(m_str) : 0; }
	inline String & Clear() { 
		if ( m_str ) 
			delete[] m_str; 
		m_str = nullptr; 
		return * this; 
	}
	inline String & Add(CSPTR str) {
		if ( str ) 
			Add(str, strlen(str));		 
		return * this;
	}
	inline String & Add(char c) {
		char buf[2]; buf[0] = c; buf[1] = '\0';
		return Add(buf);		 
	}
	inline String & NewLine() { return Add(NEWLINE); }
	inline String & operator << (CSPTR str) { return Add(str); }
	inline String & operator << (char c) { return Add(c); }
	String & Add(CSPTR ptr, size_t len);
	inline operator CSPTR() const { return m_str; }		
	CSPTR Z() const { return ZSTR((CSPTR) * this); }
	inline bool operator == (CSPTR str) const {
		if ( ! m_str ) return ! str;
		if ( ! str ) return ! m_str;
		return ! strcmp(m_str, str); 
	}
	inline int FindAnyChr(CSPTR chrs) {
		if ( ! m_str || ! chrs )
			return -1;
		CSPTR p = strpbrk(m_str, chrs);
		return p ? (p - m_str) : -1;
	}
};

class CRC32
{
private:
	static const uint32_t CRC_POLY = 0xEDB88320;
	static const uint32_t CRC_MASK = 0xD202EF8D;
private:
	uint32_t m_table[0x100];
public:
	CRC32();
	uint32_t Calc(const uint8_t * ptr, size_t len, uint32_t crc = 0);
};
extern CRC32 g_crc32;

extern "C" void MacsInit();

static const int FIRST_VIRT_IRQ = 0x0100; /// Номер, с которого начинаются виртуальные прерывания

const uint32_t INFINITE_TIMEOUT = UINT_MAX;

// конвертирует миллисекунды в тики таймера планировщика
uint32_t MsToTicks(uint32_t ms);

// конвертирует тики таймера планировщика в микросекунды
uint32_t TicksToUs(uint32_t ticks);
 
/// @enum Result
/// @brief Результат операции
/// @details Значения данного перечисления должны использоваться для результатов системных функций (ядро, драйвера)
enum Result
{
	/// операция завершена успешно
	ResultOk,

	/// операция завершена по истечению таймаута
	ResultTimeout,

	/// вызов данной операции из прерывания запрещён
	ResultErrorInterruptNotSupported,

	/// вызов данной операции из прерывания запрещён, т.к. его приоритет больше 
	/// (числовое значение меньше) чем MAX_SYSCALL_INTERRUPT_PRIORITY
	ResultErrorSysCallNotAllowed, 
	 
	/// выполнение операции не поддерживается
	ResultErrorNotSupported,

	/// аргументы операции некорректны
	ResultErrorInvalidArgs,

	/// внутреннее состояние объекта или состояние системы некорректно для выполнения операции, 
	/// например:
	/// - при вызове Scheduler::BlockCurrentTask(), текущая задача уже заблокирована
	/// - при вызове Mutex::Lock(), текущая задача уже является его владельцем
	/// - при вызове Semaphore::Signal(), текущий счётчик уже равен максимальному значению
	ResultErrorInvalidState		///< некорректное состояние
};
extern "C" CSPTR GetResultStr(Result, bool brief = false);
 
extern "C" Result SvcExecPrivileged(void* vR0, void* vR1, void* vR2, uint32_t vR3);

}	// namespace macs
 
using namespace macs;

extern "C" {
extern uint32_t SystemCoreClock;
}

#define MACS_CRASH(reason)  System::Crash(reason)
