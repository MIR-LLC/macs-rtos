    .arch armv7-m
    .cpu cortex-m1
    .syntax unified
    .thumb
    .align 4

    .globl svcMethods
    .globl SVC_Handler

.thumb_func
SVC_Handler:
		push {lr}

		// положить в R0 указатель стека, который был в момент прерывания
		mov r0, lr
		movs r1, #4
		tst r0, r1
		bne is_psp1

		mrs r0, msp
		adds r0, #4	// т.к. положили lr в MSP
		b sp_done1
is_psp1:
		mrs r0, psp
		
sp_done1:
		// считываем номер функции SVC
		ldr r1, [r0, #24]
		adds r1, #-2
		ldrb r1, [r1]
		mov lr, r1

		// вызываем SVC по номеру
		cmp r1, #0
		beq svc_init
		cmp r1, #1
		beq svc_privileged_call

		b svc_last

svc_init:
		pop {r0}	// do not need saved LR

		movs r1, #0
		subs r1, r1, #3  // Получаем псевдоадрес 0xFFFFFFFD для выхода из SVC

		mov lr, r1
		bx lr

svc_privileged_call:
		// 1 - Execute an object method with privileged access

		ldr r1, =svcMethods
		mov lr, r1							//adr lr, ... TODO check the correctness

		ldr r2, [r1]
		cmp r3, r2							// check R3 is in range
		bge svc_last						// TODO check if negative
		// put method address into R3. There is method index there currently.

		adds r3, #1
		lsls r3, #2
		ldr r3, [r1, r3]
		// восстанавливаем регистры из стека
		ldmia r0, {r0-r2}
		blx r3
		// method call should have left Result in R0, replace the value in stack
		ldr r1, [sp]		// значение EXC_RETURN лежит в стеке

		movs r2, #4
		tst r1, r2
		bne is_psp2

		mrs r1, msp			// используем основной стек, если вызов произошёл из прерывания
		str r0, [r1, #4]
		b sp_done2
is_psp2:
		mrs r1, psp			// используем стек задачи в противном случае
		str r0, [r1, #0]
sp_done2:

		pop {pc}

svc_last:
		bkpt 4	// programming error

		.align 4

    .end
