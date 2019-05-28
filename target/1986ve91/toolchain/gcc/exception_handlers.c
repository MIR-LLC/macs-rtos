#include "exception_handlers.h"
#include "cmsis_device.h"
#include <string.h>

extern void __attribute__((noreturn,weak)) _start (void);

// Default exception handlers. Override the ones here by defining your own
// handler routines in your application code.

#if defined(DEBUG)

// The DEBUG version is not naked, but has a proper stack frame,
// to allow setting breakpoints at Reset_Handler.
void __attribute__ ((section(".after_vectors"),noreturn))
Reset_Handler (void)
{
	_start();
}

#else

// The Release version is optimised to a quick branch to _start.
void __attribute__ ((section(".after_vectors"),naked))
Reset_Handler(void)
{
	asm volatile
	(
		" ldr     r0,=_start \n"
		" bx      r0"
		:
		:
		:
	);
}

#endif

void __attribute__ ((section(".after_vectors"),weak)) NMI_Handler (void)
{
#if defined(DEBUG)
	__DEBUG_BKPT();
#endif
	while (1) {
	}
}

// ----------------------------------------------------------------------------


#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)


void __attribute__ ((section(".after_vectors"),weak,naked)) HardFault_Handler (void)
{
	asm volatile(
		" tst lr,#4       \n"
		" ite eq          \n"
		" mrseq r0,msp    \n"
		" mrsne r0,psp    \n"
		" mov r1,lr       \n"
		" ldr r2,=HardFault_Handler_C \n"
		" bx r2"

		: /* Outputs */
		: /* Inputs */
		: /* Clobbers */
	);
}

void __attribute__ ((section(".after_vectors"),weak,used)) HardFault_Handler_C (
    ExceptionStackFrame* frame __attribute__((unused)), uint32_t lr __attribute__((unused)))
{
#if defined(DEBUG)
	__DEBUG_BKPT();
#endif
	while (1) {
	}
}

#endif // defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

#if defined(__ARM_ARCH_6M__)

// Hard Fault handler wrapper in assembly.
// It extracts the location of stack frame and passes it to handler
// in C as a pointer. We also pass the LR value as second
// parameter.
// (Based on Joseph Yiu's, The Definitive Guide to ARM Cortex-M0
// First Edition, Chap. 12.8, page 402).

void __attribute__ ((section(".after_vectors"),weak,naked)) HardFault_Handler (void)
{
	asm volatile(
		" movs r0,#4      \n"
		" mov r1,lr       \n"
		" tst r0,r1       \n"
		" beq 1f          \n"
		" mrs r0,psp      \n"
		" b   2f          \n"
		"1:               \n"
		" mrs r0,msp      \n"
		"2:"
		" mov r1,lr       \n"
		" ldr r2,=HardFault_Handler_C \n"
		" bx r2"

		: /* Outputs */
		: /* Inputs */
		: /* Clobbers */
	);
}

void __attribute__ ((section(".after_vectors"),weak,used)) HardFault_Handler_C (ExceptionStackFrame* frame __attribute__((unused)), uint32_t lr __attribute__((unused)))
{
	// There is no semihosting support for Cortex-M0, since on ARMv6-M
	// faults are fatal and it is not possible to return from the handler.

#if defined(DEBUG)
	__DEBUG_BKPT();
#endif
	while (1)
	{
	}
}

#endif // defined(__ARM_ARCH_6M__)

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

void __attribute__ ((section(".after_vectors"),weak)) MemManage_Handler (void)
{
#if defined(DEBUG)
	__DEBUG_BKPT();
#endif
	while (1) {
	}
}

void __attribute__ ((section(".after_vectors"),weak,naked)) BusFault_Handler (void)
{
	asm volatile(
		" tst lr,#4       \n"
		" ite eq          \n"
		" mrseq r0,msp    \n"
		" mrsne r0,psp    \n"
		" mov r1,lr       \n"
		" ldr r2,=BusFault_Handler_C \n"
		" bx r2"

		: /* Outputs */
		: /* Inputs */
		: /* Clobbers */
	);
}

void __attribute__ ((section(".after_vectors"),weak,used)) BusFault_Handler_C (
    ExceptionStackFrame* frame __attribute__((unused)), uint32_t lr __attribute__((unused)))
{

#if defined(DEBUG)
	__DEBUG_BKPT();
#endif
	while (1) {
	}
}

void __attribute__ ((section(".after_vectors"),weak,naked)) UsageFault_Handler (void)
{
	asm volatile(
		" tst lr,#4       \n"
		" ite eq          \n"
		" mrseq r0,msp    \n"
		" mrsne r0,psp    \n"
		" mov r1,lr       \n"
		" ldr r2,=UsageFault_Handler_C \n"
		" bx r2"

		: /* Outputs */
		: /* Inputs */
		: /* Clobbers */
	);
}

void __attribute__ ((section(".after_vectors"),weak,used)) UsageFault_Handler_C (
    ExceptionStackFrame* frame __attribute__((unused)), uint32_t lr __attribute__((unused)))
{

#if defined(DEBUG)
	__DEBUG_BKPT();
#endif
	while (1) {
	}
}

#endif

void __attribute__ ((section(".after_vectors"),weak)) SVC_Handler (void)
{
#if defined(DEBUG)
	__DEBUG_BKPT();
#endif
	while (1) {
	}
}

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

void __attribute__ ((section(".after_vectors"),weak)) DebugMon_Handler (void)
{
#if defined(DEBUG)
	__DEBUG_BKPT();
#endif
	while (1) {
	}
}

#endif

void __attribute__ ((section(".after_vectors"),weak)) PendSV_Handler (void)
{
#if defined(DEBUG)
	__DEBUG_BKPT();
#endif
	while (1) {
	}
}

void __attribute__ ((section(".after_vectors"),weak)) SysTick_Handler (void)
{
	// DO NOT loop, just return.
	// Useful in case someone (like STM HAL) inadvertently enables SysTick.
	;
}
