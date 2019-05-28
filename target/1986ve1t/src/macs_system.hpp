/// @file System.h
/// @brief Конфигурация аппаратной части 1986ВЕ1Т
/// @details Определяет интерфейс доступа к аппаратно-зависимому функционалу.
/// Класс System должен иметь специфичную для используемого аппаратного обеспечения реализацию.
/// @copyright AstroSoft Ltd, 2016

#pragma once

#include "MDR1986VE1T.h"

#include "macs_platform.hpp"
#include "MDR32F9Qx_port.h"
#include "MDR32F9Qx_uart.h"
#include "MDR32F9Qx_rst_clk.h"

#ifndef MACS_HEAP_SIZE
	#define MACS_HEAP_SIZE (16 KILO_B)
#endif	

#ifndef MACS_INIT_CPU_FREQ_MHZ
  #define MACS_INIT_CPU_FREQ_MHZ 144
#endif	
/// @brief Класс, содержащий реализацию платформозависимых методов
class System : public SystemBase
{
public:
	static const uint32_t HEAP_SIZE = MACS_HEAP_SIZE;

	static void InitCpu();
	static void HardFaultHandler();
};
