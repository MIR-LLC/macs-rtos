/// @file macs_platform.hpp
/// @brief Аппаратно-зависимый код.
/// @details Содержит объявления классов и т.п., реализация которых зависит от конкретной аппаратной платформы.
/// @copyright AstroSoft Ltd, 2016

#pragma once

#include <stdint.h>
#include "macs_common.hpp"

// Номер версии ядра процессора
#define MACS_CORTEX_M0		1
#define MACS_CORTEX_M0_P	5
#define MACS_CORTEX_M1		10
#define MACS_CORTEX_M3		30
#define MACS_CORTEX_M4		40

// По ИД платы определяется ядро и системные файлы
#if defined(STM32F429xx)
#define MACS_MCU_CORE  MACS_CORTEX_M4
#define MACS_PLATFORM_INCLUDE_1  "stm32f4xx.h"
#define MACS_PLATFORM_INCLUDE_2  "stm32f4xx_hal.h"
#endif	

#if defined(MDR1986VE9x)
#define MACS_MCU_CORE  MACS_CORTEX_M3
#define MACS_PLATFORM_INCLUDE_1  "MDR32Fx.h"
#endif

#ifdef MDR1986VE1T
#define MACS_MCU_CORE  MACS_CORTEX_M1
#define MACS_PLATFORM_INCLUDE_1  "MDR1986VE1T.h"
#endif	

#include "macs_stack_frame.hpp"
#include "macs_cortex_m.h"

#if MACS_USE_MPU
typedef enum
{
	MPU_ZERO_ADR_MINE = 1,
	MPU_PROC_STACK_MINE,
	MPU_MAIN_STACK_MINE
}MPU_MINE_NUM;

extern void MPU_Init();
extern void MPU_SetMine(MPU_MINE_NUM, uint32_t adr);
extern void MPU_RemoveMine(MPU_MINE_NUM);
#endif		

class StackPtr
{
private:
	// Маркер вершины стека (случайная маска) - детектор переполнения стека
	static const uint32_t TOP_MARKER = 0xA52E3FC1;
public:
	enum CHECK_RES
	{
		SP_OK = 0,
		SP_OVERFLOW,
		SP_UNDERFLOW,
		SP_CORRUPTED
	};
	uint32_t * m_sp;
public:
	StackPtr(uint32_t * sp = nullptr)
	{
		Set(sp);
	}
	inline void Set(uint32_t * sp)
	{
		m_sp = sp;
	}
	inline void Zero()
	{
		Set(nullptr);
	}
	size_t GetVirginLen(StackPtr marg) const;
	CHECK_RES Check(StackPtr marg, size_t len);
	void Instrument(StackPtr marg, bool do_full);
#if MACS_MPU_PROTECT_STACK		
	inline void SetMpuMine() {
		MPU_SetMine(MPU_PROC_STACK_MINE, ((uint32_t) m_sp & ~0x1F) - 0x20);
	}
#endif
private:
	static void FillWithMark(uint32_t * ptr, uint32_t * lim);
	static size_t GetVirginLen(uint32_t * beg, uint32_t * lim);
};

class TaskStack
{
private:
	/// @brief Размер области стека, достаточной для экономной работы задачи
	static const size_t WORK_SIZE = 0x10;
public:
	/// @brief Минимальный размер стека в словах.
	static const size_t MIN_SIZE
#if (__FPU_USED == 1)
	/// @details Размер складывается из:
	/// + аппаратно сохраняемой части контекста в 0x1A или 0x1B слов (в зависимости от выравнивания DWORD),
	/// + программно сохраняемая часть контекста в 0x19 слов (что даёт максимум 0x34 слова на контекст),
	/// + 0x10 слов для нужд самой задачи.
	= 0x34 + WORK_SIZE;
#else
	/// @details Размер складывается из:
	/// + аппаратно сохраняемой части контекста в 0x08 или 0x09 слов (в зависимости от выравнивания DWORD),
	/// + программно сохраняемая часть контекста в 0x09 слов (что даёт максимум 0x12 слов на контекст),
	/// + 0x10 слов для нужд самой задачи.
	= 0x12 + WORK_SIZE;
#endif
	/// @brief Размер стека, достаточный для работы большинства задач
	static const size_t ENOUGH_SIZE
#if MACS_MCU_CORE == MACS_CORTEX_M3
	= 500;
#else
	= 350;
#endif
private:
	// Размер области стека, предназначенной для затирания в случае переполнения
#if MACS_MPU_PROTECT_STACK		
	static const size_t GUARD_SIZE = MAX(WORK_SIZE, (2 * 32/4) - 1);
#else
	static const size_t GUARD_SIZE = WORK_SIZE;
#endif
	// Минимальный остаток стека в словах до автоматического увеличения.
	static const size_t MIN_REST = 2 * WORK_SIZE;
	// Шаг увеличения стека в словах
	static const int GROW_SIZE = MIN_SIZE;
public:
	// Максимальный размер стека в словах.
	static const size_t MAX_SIZE = MACS_MAX_STACK_SIZE - GUARD_SIZE;
private:
	// признак стека во внешней памяти
	bool m_is_alien_mem;
	// размер стека в словах (может быть не равным размеру области памяти!)
	size_t m_len;
	// область памяти для стека
	uint32_t * m_memory;
	// граница указателя стека, за которую он не должен выходить
	StackPtr m_margin;
	void PreparePlatformSpecific(size_t len, void * this_ptr, void (*run_func)(void), void (*exit_func)(void));
public:
	// указатель на вершину стека
	StackPtr m_top;
public:
	TaskStack()
	{
		m_is_alien_mem = false;
		m_len = 0;
		m_memory = nullptr;
	}
	~TaskStack()
	{
		Free();
	}
	void Build(size_t len, uint32_t * mem = nullptr);
	void BuildPlatformSpecific(size_t guard, size_t len);
	void Prepare(size_t len, void * this_ptr, void (*run_func)(void), void (*exit_func)(void));
	void Free();
	void Instrument()
	{
		m_top.Instrument(m_margin, true);
	}
	inline size_t GetLen() const
	{
		return m_len;
	}
	inline size_t GetUsage() const
	{
		return m_len - m_top.GetVirginLen(m_margin);
	}
	bool Check();
#if MACS_MPU_PROTECT_STACK		
	inline void SetMpuMine() {m_margin.SetMpuMine();}
#endif
};

/// @brief Базовый класс для описания платформозависимых методов.
/// @details Данный класс содержит описание методов, которые используются ядром ОС и реализация
/// которых зависит от конечной аппаратной платформы. Модули с реализацией этих методов находятся
/// в соответствующем подкаталоге каталога Targets.
class SystemBase
{
private:
	static uint32_t m_tick_rate_hz;
	static const uint m_stack_alignment;
public:
	// Установлен только признак состояния Thumb, т.к. данный процессор поддерживает только Thumb инструкции.
	// Попытка сброса данного бита приведёт к генерации исключения отказа.
	static const uint32_t INITIAL_XPSR = 0x01000000;

	// максимальный приоритет прерываний, из которых можно делать вызовы функций ядра
	// (планировщик, синхронизирующие примитивы)
	static const int MAX_SYSCALL_INTERRUPT_PRIORITY = 5;

	// номер с которого начинаются НЕ системные прерывания
	// необходимо чтобы можно было по значению IPSR определить соответствующее ему
	// значение в перечислении IRQn_Type
	static const int FIRST_USER_INTERRUPT_NUMBER = 16;

	static const uint32_t INTERRUPT_MIN_PRIORITY = 0xFFu;
	static const uint32_t CONTROL_UNPRIV_FLAG = 0x01;
#if MACS_MPU_PROTECT_STACK
	static const size_t MacsMainStackSize = MACS_MAIN_STACK_SIZE; // Размер стека планировщика (в словах).
	static uint32_t * MacsMainStackBottom;
#endif

public:

	// Запрещает прерывания, из которых можно производить вызовы ядра (критическая секция).
	// Возвращает предыдущее состояние маски.
	static uint32_t DisableIrq();

	// Восстанавливает маску прерываний.
	// Используется чтобы разрешить прерывания при выходе из критической секции.
	static void EnableIrq(uint32_t mask);

	// Устанавливает приоритет прерывания.
	// Используется чтобы прерывание не могло пробить критическую секцию.
	// В таком случае из его обработчика можно вызывать ядро.
	static void SetIrqPriority(int irq_num, uint priority);

	// Возвращает номер текущего прерывания.
	// Результат совместим с CMSIS IRQn_Type, например, вне прерывания вернется -16.
	static int CurIrqNum();

	// Определяет находимся ли мы в процессе обработки SVC.
	static bool IsInSysCall();

	// Делает настройки для использования задачи-обработчика TaskIrq, назначенного на заданное прерывание
	// vector - инициализировать вектор прерывания, enable - разрешать данное прерывание
	// истина, если все удалось
	static bool inline SetUpIrqHandling(int irq_num, bool vector, bool enable)
	{
		return false;
	} // todo реализовать для Cortex ?

	// Определяет находимся ли мы в обработчике прерывания.
	// Вызов SVC также считается прерыванием.
	static bool IsInInterrupt();

	// Возвращает истину, если включен привилегированный режим.
	static bool IsInPrivMode();

	// Возвращает истину, если включен привилегированный режим или находимся в обработчике прерывания
	// Эквивалентно выражению (IsInInterrupt() || IsInPrivMode())
	static inline bool IsInPrivOrIrq()
	{
		return IsInPrivMode() || IsInInterrupt();
	}

	// Возвращает истину, если в данный момент разрешены вызовы ядра.
	// Если находимся в режиме потока, то вернёт true
	// Если находимся в режиме прерывания и его приоритет больше (числовое значение меньше)
	// чем MAX_SYSCALL_INTERRUPT_PRIORITY, то вернёт false. Иначе вернёт true.
	static bool IsSysCallAllowed();

	// Возвращает необходимое выравнивание стека: 0 - без выравнивания, 1 - четность, 2 - кратно 4 и т.д.
	static inline uint GetStackAlignment()
	{
		return m_stack_alignment;
	}

	// Возвращает истину, если в данный момент в качестве указателя стека используется MSP.
	static bool IsInMspMode();

	// Возвращает текущее значение указателя стека MSP.
	static uint32_t GetMsp();

	// Устанавливает значение указателя стека PSP.
	static void SetPsp(StackPtr sp);

	// Переключение на первую задачу при старте планировщика
	static void FirstSwitchToTask(StackPtr sp, bool is_privileged);

	// Устанавливает режим режим выполнения (привилегированный или непривилегированный, в зависимости от параметра)
	// для задачи при выходе из переключения контекста.
	static void SetPrivMode(bool is_on);

	// Выдает запрос на переключение контекста.
	static void SwitchContext();

	// Производит первичную инициализацию планировщика.
	static bool InitScheduler();

#if MACS_USE_MPU
	// Первичная инициализация MPU.
	static void MpuInit ();

	// Установка региона MPU минимального размера по указанному адресу с защитой от записи.
	// Адрес должен быть выровнен надлежащим образом.
	// Параметр rnum задает номер региона.
	static void MpuSetMine (uint32_t rnum, uint32_t adr);

	// Снятие указанного региона MPU.
	static void MpuRemoveMine (uint32_t rnum);
#endif

	// Выдает текущую частоту процессора в герцах.
	static inline uint32_t GetCpuFreq()
	{
		return SystemCoreClock;
	}

	// Выдает текущую частоту следования тиков ОС в герцах.
	static inline uint32_t GetTickRate()
	{
		return m_tick_rate_hz;
	}

	/// @brief Выдает период системных тиков в миллисекундах
	/// @return Значение с плавающей точкой - период системных тиков в миллисекундах
	static inline float GetTickPeriod()
	{
		return ((float)1000) / m_tick_rate_hz;
	} // дефект 40891

	// Устанавливает частоту следования тиков ОС в герцах.
	static bool SetTickRate(uint32_t rate_hz);

	/// @brief Устанавливает период системных тиков в миллисекундах
	/// @param inPeriod - значение с плавающей точкой - период системных тиков в миллисекундах
	/// @return Истина, если удалось установить заданный период
	static inline bool SetTickPeriod(float inPeriod)
	{
		return SetTickRate(1000 / inPeriod);
	} // дефект 40891

	// Возвращает текущее значение счетчика тактов процессора.
	// При переполнении счет продолжается с нуля.
	// Вызов возможен только из привилегированного режима.
	static ulong GetCurCpuTick();

	// Устанавливает текущее значения счетчика тактов процессора.
	// Вызов возможен только из привилегированного режима.
	static void SetCurCpuTick(ulong tk);

	// Запрашивает текущее значение счетчика тактов процессора.
	// Может выполняться из непривилегированного режима, в этом случае время отклика сильно увеличивается.
	static ulong AskCurCpuTick();

	static inline ulong CpuTicksToNs(ulong cpu_ticks)
	{
		_ASSERT(cpu_ticks <= ULONG_MAX / 1000);
		return (1000 * cpu_ticks) / (GetCpuFreq() / 1000000);
	}
	static inline ulong CpuTicksToUs(ulong cpu_ticks)
	{
		return cpu_ticks / (GetCpuFreq() / 1000000);
	}
	static inline ulong CpuToOsTicks(ulong cpu_ticks)
	{
		return cpu_ticks / (GetCpuFreq() / GetTickRate());
	}

	static inline ulong ReadUs()
	{
		return CpuTicksToUs(GetCurCpuTick());
	}
	static inline ulong ReadMs()
	{
		return ReadUs() / 1000;
	}

	static inline void WaitNs(ulong delay)
	{
		delay = ((GetCpuFreq() / 1000000) * delay) / 1000;
		ulong start_tick = GetCurCpuTick();
		while (GetCurCpuTick() - start_tick < delay)
			;
	}
	static inline void WaitUs(ulong delay_us)
	{
		_ASSERT(delay_us <= ULONG_MAX / 1000);
		WaitNs(1000 * delay_us);
	}
	static inline void WaitMs(ulong delay_ms)
	{
		_ASSERT(delay_ms <= ULONG_MAX / 1000000);
		WaitNs(1000000 * delay_ms);
	}

	// Выполняет аппаратный сброс процессора.
	// Внимание! Вызов этого метода приведет к немедленному завершению работы и перезапуску приложения.
	static void McuReset();

	// Немедленная остановка работы при возникновении аварийной ситуации.
	static inline void Crash(ALARM_REASON)
	{
		MACS_BKPT(1);
	}

	// Немедленное переключение контекста, без традиционного механизма системного прерывания.
	// Вызывается из критической секции (пока что только из DeleteTask_Priv).
	static void InternalSwitchContext();

	// Перевод процессора в спящий режим.
	static void EnterSleepMode();
};

#if (MACS_MCU_CORE < MACS_CORTEX_M3) || (MACS_MCU_CORE == MACS_TS201)
extern uint8_t MACS_LDREXB(const void * ptr);
extern uint32_t MACS_LDREXW(const void * ptr);
extern uint32_t MACS_STREXB(uint8_t val, void * ptr);
extern uint32_t MACS_STREXW(uint32_t val, void * ptr);
#else
#define MACS_LDREXB  __LDREXB
#define MACS_LDREXW  __LDREXW
#define MACS_STREXB  __STREXB
#define MACS_STREXW  __STREXW
#endif
