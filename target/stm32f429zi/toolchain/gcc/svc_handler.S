    .arch armv7-m
    .cpu cortex-m4
    .syntax unified
    .thumb
    .align 4

    .set THREAD_PSP_RETURN, 0xFFFFFFFD

    .globl svcMethods
    .globl SVC_Handler

.thumb_func
SVC_Handler:
    push {lr}

    // положить в R0 указатель стека, который был в момент прерывания
    tst lr, #4    // TODO named constant?
    itte eq
    mrseq r0, msp
    addeq r0, #4    // because we pushed lr into MSP
    mrsne r0, psp

    // считываем номер функции SVC
    ldr lr, [r0, #24]
    ldrb lr, [lr, #-2]

    // check SVC number is in range
    cmp lr, #2
    bge svc_last    // exit if greater than supported

    tbb [pc, lr]
branchtable:
    .byte ((svc_init - branchtable) / 2)
    .byte ((svc_privileged_call - branchtable) / 2)

    .align 4

svc_init:
    pop {r0}    // do not need saved LR
    mov lr, #THREAD_PSP_RETURN        //  TODO Shouldn't we remove HW Stack Frame because we are not going to return back ever ?

    bx lr

svc_privileged_call:
    // 1 - Execute an object method with privileged access
    // восстанавливаем регистры из стека,
    // см. стр. 333-334 в руководстве по процессору
    // (хотя так и не понял причину порчи регистров)
    ldmia r0, {r0-r3}

    ldr lr, =svcMethods        //adr lr, ... TODO check the correctness
    ldr r12, [lr], #4        // R12 будет восстановлено при возврате из прерывания - не жалко
    cmp r3, r12                // check R3 is in range
    bge svc_last            // TODO check if negative
    // put method address into R3. There is method index there currently.
    ldr r3, [lr, r3, lsl #2]
    blx r3
    // method call should have left Result in R0, replace the value in stack
    ldr r1, [sp]           // значение EXC_RETURN лежит в стеке
    tst r1, #0x4           // проверка используемого стека
    ittee eq
    mrseq r1, msp          // используем основной стек, если вызов произошёл из прерывания
    streq r0, [r1, #4]
    mrsne r1, psp          // используем стек задачи в противном случае
    strne r0, [r1, #0]

    pop {pc}

svc_last:
    bkpt 4    // programming error

    .end
