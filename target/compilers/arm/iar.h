#pragma once

#define MACS_BKPT(num) __asm volatile ("bkpt %0" : : "i"(num))
