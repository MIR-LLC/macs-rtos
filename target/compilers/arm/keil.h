#pragma once

#define MACS_BKPT(num) __asm volatile ("bkpt "#num)
