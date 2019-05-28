#pragma once
#if defined ( __GNUC__ )
#define KERNEL_BKPT(num)  __asm volatile ("bkpt %0" : : "i"(num))
#else
#error Wrong/unknown compiler!
#endif	
