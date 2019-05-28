    .syntax unified
    .arch armv7-m
    .cpu cortex-m1
    .thumb
    .align 4

	.globl Hard_Fault_Handler_C
	.globl HardFault_Handler

.thumb_func
HardFault_Handler:
		mov r0, lr
		movs r1, #4
		tst r0, r1
		bne is_psp

		mrs r0, msp
		b sp_done
is_psp:
		mrs r0, psp
sp_done:
		bl Hard_Fault_Handler_C
		bx lr

	.end
