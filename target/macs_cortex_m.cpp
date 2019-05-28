/** @copyright AstroSoft Ltd */

#include "macs_system.hpp"
#include "macs_scheduler.hpp"

#if MACS_MCU_CORE >= MACS_CORTEX_M3
static const uint32_t DISABLE_INTERRUPTS_MASK = System::MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - __NVIC_PRIO_BITS);

uint32_t SystemBase::DisableIrq()
{
	const uint32_t prev_mask = __get_BASEPRI();

	__set_BASEPRI(DISABLE_INTERRUPTS_MASK);
	__DSB();
	__ISB();

	return prev_mask;
}

void SystemBase::EnableIrq(uint32_t mask)
{
	__set_BASEPRI(mask);
	__DSB();
	__ISB();
}
#else
#if defined(__ICCARM__) || defined(__GNUC__)
uint32_t SystemBase::DisableIrq()
{
	int ret;
	asm("mrs %0, PRIMASK" : "=r"(ret));
	asm("cpsid i");
	return ret;
}

void SystemBase::EnableIrq(uint32_t mask)
{
	asm("msr PRIMASK, %0" : : "r"(mask));
}
#else
__asm uint32_t SystemBase::DisableIrq()
{
	mrs r0, PRIMASK
	cpsid i
	bx lr
}

__asm void SystemBase::EnableIrq(uint32_t mask)
{
	msr PRIMASK, r0
	bx lr
}
#endif
#endif

bool SystemBase::SetTickRate(uint32_t rate_hz)
{
	if (rate_hz == 0 || SystemCoreClock / rate_hz <= 1)
		return false;
	uint32_t ticks = (SystemCoreClock / rate_hz) - 1;
	if (ticks > SysTick_LOAD_RELOAD_Msk)
		return false;

	SysTick->LOAD = ticks;
	SysTick->VAL = 0;
	// запустить системный таймер (внутренний источник тактов, прерывания активированы)
	SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;

	m_tick_rate_hz = rate_hz;

	return true;
}

bool SystemBase::InitScheduler()
{
	// устанавливается минимальный приоритет для того чтобы данные прерывания не могли быть вложенными, 
	// т.е. не возникали во время выполнения других прерываний и друг друга;
	// проблема в том что, при завершении операции переключения контекста (прерывание PendSV) 
	// происходит возврат в режим потока, но если PendSV было вложенным, то возникнет исключение Usage fault
	NVIC_SetPriority(PendSV_IRQn, INTERRUPT_MIN_PRIORITY);
	NVIC_SetPriority(SysTick_IRQn, INTERRUPT_MIN_PRIORITY);

	SystemCoreClockUpdate();

	return SetTickRate(m_tick_rate_hz);
}

void SystemBase::SwitchContext()
{
	// вызываем переключение контекста посредством прерывания PendSV
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	__DSB();
	__ISB();
}

#if MACS_MCU_CORE >= MACS_CORTEX_M3
ulong SystemBase::GetCurCpuTick()
{
	return DWT->CYCCNT;
}
void SystemBase::SetCurCpuTick(ulong tk)
{
	DWT->CYCCNT = tk;
}
#else
ulong SystemBase::GetCurCpuTick()
{
	return 0;
}
void SystemBase::SetCurCpuTick(ulong tk)
{
}
#endif

#if MACS_USE_MPU
uint32_t * SystemBase::MacsMainStackBottom;

#ifndef MPU_RASR_XN_Pos	
#define MPU_RASR_XN_Pos                    28U                                            /*!< MPU RASR: ATTRS.XN Position */
#endif	

#ifndef MPU_RASR_AP_Pos
#define MPU_RASR_AP_Pos                    24U                                            /*!< MPU RASR: ATTRS.AP Position */
#endif	

static inline bool IsMpuPresent()
{
	return !!(MPU->TYPE & MPU_TYPE_DREGION_Msk);
}

void SystemBase::MpuInit()
{
	if (!IsMpuPresent())
		return;

	MPU->RNR = 0;
	MPU->RBAR = 0x00000000U | (0 << MPU_RBAR_VALID_Pos);
	MPU->RASR = (0 << MPU_RASR_XN_Pos) | (0x3 << MPU_RASR_AP_Pos) | (0x1F << MPU_RASR_SIZE_Pos) | (1 << MPU_RASR_ENABLE_Pos);
	__DSB();

	SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;

	MPU->CTRL = (1 << MPU_CTRL_ENABLE_Pos);
}

void SystemBase::MpuSetMine(uint32_t rnum, uint32_t adr)
{
	if (!IsMpuPresent())
		return;

	MPU->RNR = rnum;
	MPU->RBAR = adr | (0 << MPU_RBAR_VALID_Pos);
	MPU->RASR = (1 << MPU_RASR_XN_Pos) | (0x0 << MPU_RASR_AP_Pos) | (0x4 << MPU_RASR_SIZE_Pos) | (1 << MPU_RASR_ENABLE_Pos);
	__DSB();
}

void SystemBase::MpuRemoveMine(uint32_t rnum)
{
	if (!IsMpuPresent())
		return;

	MPU->RNR = rnum;
	MPU->RBAR = 0x00000000U | (0 << MPU_RBAR_VALID_Pos);
	MPU->RASR = (0 << MPU_RASR_XN_Pos) | (0x3 << MPU_RASR_AP_Pos) | (0x1F << MPU_RASR_SIZE_Pos) | (0 << MPU_RASR_ENABLE_Pos);
	__DSB();
}
#endif	// #if MACS_USE_MPU

// Примечание: руководство по процессору не рекомендует изменять выравнивание стека после
// инициализации, поэтому флаг сохраняется в переменной для дальнейшего использования
const uint SystemBase::m_stack_alignment = (SCB->CCR & SCB_CCR_STKALIGN_Msk ? 1 : 0);

void SystemBase::SetIrqPriority(int irq_num, uint priority)
{
	NVIC_SetPriority((IRQn_Type)irq_num, priority);
}

bool SystemBase::IsInInterrupt()
{
	return __get_IPSR() != 0;
}

int SystemBase::CurIrqNum()
{
	return __get_IPSR() - FIRST_USER_INTERRUPT_NUMBER;
}

bool SystemBase::IsInSysCall()
{
	return CurIrqNum() == SVCall_IRQn;
}

bool SystemBase::IsInPrivMode()
{
	return (__get_CONTROL() & 0x1) == 0;
}

bool SystemBase::IsSysCallAllowed()
{
	uint32_t irqNum = __get_IPSR();
	IRQn_Type cmsisIrqNum = (IRQn_Type)(irqNum - FIRST_USER_INTERRUPT_NUMBER);

	if (irqNum == 0 || cmsisIrqNum == SVCall_IRQn) {
		// находимся в режиме потока или выполняем системный вызов из потока
		return true;
	}

	// для следующих исключений нельзя изменить приоритет, и он меньше 0:
	// #1 Reset
	// #2 NMI
	// #3 Hard Fault
	// но MAX_SYSCALL_INTERRUPT_PRIORITY не может быть меньше 1, поэтому вызов
	// функций ядра из этих прерываний невозможен
	if (irqNum <= 3) {
		return false;
	}

	return NVIC_GetPriority(cmsisIrqNum) >= MAX_SYSCALL_INTERRUPT_PRIORITY;
}

bool SystemBase::IsInMspMode()
{
	return (__get_CONTROL() & 0x2) == 0;
}

uint32_t SystemBase::GetMsp()
{
	return __get_MSP();
}

void SystemBase::SetPsp(StackPtr sp)
{
	__set_PSP((uint32_t)sp.m_sp);
}

void SystemBase::SetPrivMode(bool is_on)
{
	uint32_t ctrl = __get_CONTROL();

	if (is_on)
		ctrl &= ~CONTROL_UNPRIV_FLAG;
	else
		ctrl |= CONTROL_UNPRIV_FLAG;

	__set_CONTROL(ctrl);
}

extern "C" void SvcInitScheduler();

void SystemBase::FirstSwitchToTask(StackPtr sp, bool is_privileged)
{
	// устанавливаем указатель PSP на стековый фрейм прерывания, т.к. по завершении
	// прерывания SVC мы сразу провалимся в первую задачу, минуя PendSV_Handler,
	// и со стека будут взяты только основные регистры (r0-r3, r12, lr, pc, psr)

	System::SetPsp(StackPtr((uint32_t *)StackFramePtr::getHWFramePtr(sp.m_sp)));

	System::SetPrivMode(is_privileged);

	// Переключение на первую задачу
	SvcInitScheduler();
}

void SystemBase::McuReset()
{
	NVIC_SystemReset();
}

void SystemBase::InternalSwitchContext()
{
	Sch().TryContextSwitch();
}

void SystemBase::EnterSleepMode()
{
	__DSB();
	__WFI();
}

StackPtr::CHECK_RES StackPtr::Check(StackPtr marg, size_t len)
{
	if (*marg.m_sp != StackPtr::TOP_MARKER)
		return SP_CORRUPTED;
	int rest = m_sp - marg.m_sp;
	if (rest > len)
		return SP_UNDERFLOW; // Указатель каким-то чудом ушел за дно стека
	if (rest < 0)
		return SP_OVERFLOW; // Стек переполнился - указатель за вершиной стека

	return SP_OK;
}

void StackPtr::Instrument(StackPtr marg, bool do_full)
{
	if (do_full)
		FillWithMark(marg.m_sp, m_sp);
	else
		*marg.m_sp = TOP_MARKER; // Кладем маркер на вершину стека - переполнение его затрет
}

void TaskStack::PreparePlatformSpecific(size_t len, void * this_ptr, void (*run_func)(void), void (*exit_func)(void))
{
	// проверка выравнивания m_sp в соответствии с SCB->CCR и корректировка значения при необходимости
	if ((reinterpret_cast<uint32_t>(m_top.m_sp) & 0x7) != 0 && SystemBase::GetStackAlignment())
		--m_top.m_sp;

	m_top.m_sp -= StackFramePtr::getStackFrameSize() / sizeof(uint32_t);

	// EXC_RETURN должен лежать на вершине стека
	*m_top.m_sp = StackFramePtr::getInitial_EXC_RETURN();

	HardwareStackFrame & hw_frame = *StackFramePtr::getHWFramePtr(m_top.m_sp);

	hw_frame.XPSR = SystemBase::INITIAL_XPSR;
	hw_frame.PC = reinterpret_cast<uint32_t>(run_func);

	// если задача завершится, то попадём в функцию exit_func
	hw_frame.LR = reinterpret_cast<uint32_t>(exit_func);

	// R0 это первый аргумент, поэтому this
	hw_frame.R0 = reinterpret_cast<uint32_t>(this_ptr);
}

void TaskStack::BuildPlatformSpecific(size_t guard, size_t len)
{
	if (!m_is_alien_mem) {
		m_len = (len < MIN_SIZE ? MIN_SIZE : (len <= MAX_SIZE ? len : MAX_SIZE));
		m_memory = new uint32_t[m_len + guard];
	} else {
		_ASSERT(len >= MIN_SIZE + guard && len <= MAX_SIZE);
		m_len = len - guard;
	}

	// используется убывающий стек, память для стекового фрейма сразу резервируются
	// так что m_top будет указывать на память перед ним
	m_margin.Set(m_memory + guard);
	m_top.Set(m_margin.m_sp + m_len);
}

size_t StackPtr::GetVirginLen(StackPtr marg) const
{
	return GetVirginLen(marg.m_sp, m_sp);
}

namespace macs
{

extern "C" void SysTick_Handler()
{
	const bool contextSwitchRequired = SchedulerSysTickHandler();

	if (contextSwitchRequired)
		System::SwitchContext();
}

} // namespace macs

#if MACS_MCU_CORE < MACS_CORTEX_M3
uint8_t MACS_LDREXB(const void * ptr)
{
	Sch().Pause(true);
	return *(uint8_t *)ptr;
}

uint32_t MACS_LDREXW(const void * ptr)
{
	Sch().Pause(true);
	return *(uint32_t *)ptr;
}

uint32_t MACS_STREXB(uint8_t val, void * ptr)
{
	*(uint8_t *)ptr = val;
	Sch().Pause(false);
	return 0;
}

uint32_t MACS_STREXW(uint32_t val, void * ptr)
{
	*(uint32_t *)ptr = val;
	Sch().Pause(false);
	return 0;
}
#endif	
