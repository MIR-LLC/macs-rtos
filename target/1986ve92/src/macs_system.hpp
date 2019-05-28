/** @copyright AstroSoft Ltd */
#pragma once

#define USE_MDR1986VE9x

#include "macs_platform.hpp"
#include "MDR32Fx.h"

#ifndef MACS_HEAP_SIZE
#define MACS_HEAP_SIZE 16384
#endif	

class System: public SystemBase
{
public:
	static const uint32_t HEAP_SIZE = MACS_HEAP_SIZE;

	static void InitCpu();
	static void HardFaultHandler();
};
