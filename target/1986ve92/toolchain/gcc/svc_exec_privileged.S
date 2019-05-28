    .arch armv7-m
    .cpu cortex-m3
    .syntax unified
    .thumb
    .align 2

    .globl SvcExecPrivileged
    .globl SvcInitScheduler

.thumb_func
SvcExecPrivileged:
    svc 1
    bx lr

.thumb_func
SvcInitScheduler:
    svc 0
    bx lr

    .end
