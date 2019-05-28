        .syntax unified
        .arch armv7-m
        .cpu cortex-m1
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

save_R4_11:
		subs r0, #32
		mov r1, r0
		stmia r1!, {r4-r7}
		mov r4, r8
		mov r5, r9
		mov r6, r10
		mov r7, r11
		stmia r1!, {r4-r7}

		mov r1, lr
		subs r0, #4
		str r1, [r0]
		// переключаемся на следующую задачу
		bl SchedulerSwitchContext

		// SP новой задачи теперь в R0
		// восстанавливаем контекст
		adds r0, #20
		ldmia r0!, {r4-r7}
		mov r8, r4
		mov r9, r5
		mov r10, r6
		mov r11, r7

		subs r0, #36
		ldmia r0!, {r1,r4-r7}
		mov lr, r1

		adds r0, #16
		msr psp, r0

		bx lr

		.align 4
		.end
