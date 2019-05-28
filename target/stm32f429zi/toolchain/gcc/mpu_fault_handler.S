        .syntax unified
        .arch armv7-m
        .cpu cortex-m4
        .thumb
        .align 4

		.globl MemManage_Handler_C
		.globl MemManage_Handler

.thumb_func
MemManage_Handler:
//		tst lr, #4
//		ite eq
//		mrseq r0, msp
//		mrsne r0, psp
//		bl MemManage_Handler_C
//		bx lr

		// code to reanimate system after memory fault
		push {r4-r7}
		// load msp to r0
		mrs r0, msp
		// load last EXEC_RETURN to r1
		ldr r1, [r0,#0x14]

		// check the enter stack pointer type
		tst r1, #4
		ite eq
		mrseq r0, msp
		mrsne r0, psp
		
		// *** load the stacked PC value ***
		ldr r4, [r0, #0x18]

		// сохраняем указатель стека, чтобы вызвать обработчик
		mov r5, r0
		// сохраняем LR, чтобы вызвать обработчик
		mov r6, lr
		bl MemManage_Handler_C
		// восстанавливаем испорченный LR
		mov lr, r6

		// read the status register to get the fault reason
		ldr r2, =0xE000ED28
		ldr r3, [r2]
		// Adjust the stacked PC value
		add r4,r4, #0x2
		// store back the stacked PC
		str r4,[r5, #0x18]
		// Clear status
		str r3, [r2]
		// return
		pop {r4-r7}

		// *** try to return on next command ***
		bx lr

        .end
