/// @file macs_system.hpp
/// @brief Конфигурация аппаратной части.
/// @details Определяет интерфейс доступа к аппаратно-зависимому функционалу.
/// Класс System должен иметь специфичную для используемого аппаратного обеспечения реализацию.
/// @copyright AstroSoft Ltd, 2016

#pragma once

#include "macs_platform.hpp"
#include "MDR32Fx.h"

#ifndef MACS_HEAP_SIZE
	#define MACS_HEAP_SIZE (16 KILO_B)
#endif	

#ifndef MDR1986VE91VE94
	#define MDR1986VE91VE94 1
#endif

class System: public SystemBase
{
public:
	static const uint32_t HEAP_SIZE = MACS_HEAP_SIZE;

	static void InitCpu();
	static void HardFaultHandler();
};
