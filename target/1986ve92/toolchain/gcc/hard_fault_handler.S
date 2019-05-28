    .syntax unified
    .arch armv7-m
    .cpu cortex-m3
    .thumb
    .align 4

	.globl Hard_Fault_Handler_C
	.globl HardFault_Handler

.thumb_func
HardFault_Handler:
	tst lr, #4
	ite eq
	mrseq r0, msp
	mrsne r0, psp
	bl Hard_Fault_Handler_C
	bx lr

	.end
