        .syntax unified
        .arch armv7-m
        .cpu cortex-m3
        .thumb
        .align 4

// На ARM Cortex-M PendSV это специальное прерывание,
// предусмотренное для переключений контекста

		.globl SchedulerSwitchContext
		.globl PendSV_Handler

.thumb_func
PendSV_Handler:
		// сохраняем контекст
		mrs r0, psp
	.if __FPU_USED == 1
		tst lr, #0x10	//  check frame type, 0 for FPU TODO constant
		bne save_R4_11
		vstmdb r0!, {s16-s31}
	.endif

save_R4_11:
		stmdb r0!, {r4-r11}
		str lr, [r0, #-4]!

		// переключаемся на следующую задачу
		bl SchedulerSwitchContext

		// new task's SP is in R0 now. восстанавливаем контекст
		ldr lr, [r0], #4
		ldmia r0!, {r4-r11}
	.if	__FPU_USED == 1
		tst lr, #0x10	//  check frame type, 0 for FPU TODO constant
		bne rst_PSP
		vldmia r0!, {s16-s31}
	.endif

rst_PSP:
		msr psp, r0
		bx lr

		.align 4
		.end
