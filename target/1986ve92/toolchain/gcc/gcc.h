#pragma once

#define MAKS_BKPT(num)  __asm volatile ("bkpt %0" : : "i"(num))
