/// @file System.h
/// @brief Конфигурация аппаратной части.
/// @details Класс System имеет специфичную для Миландр 1986ВЕ1Т реализацию.
/// @copyright AstroSoft Ltd, 2016

/// @file System.h
/// @brief Конфигурация аппаратной части.
/// @details Класс System имеет специфичную для Миландр 1986ВЕ9х реализацию.
/// @copyright AstroSoft Ltd, 2016

#include "macs_system.hpp"

//!!! move back #include "MDR32F9Qx_eeprom.h"

extern "C" void Demo_Init(void);

#define ALL_PORTS_CLK   (RST_CLK_PCLK_PORTA | RST_CLK_PCLK_PORTB | \
                         RST_CLK_PCLK_PORTC | RST_CLK_PCLK_PORTD | \
                         RST_CLK_PCLK_PORTE | RST_CLK_PCLK_PORTF)

void System::InitCpu()
{
#ifndef MACS_CUSTOM_SYS_INIT  
	/* Enable HSE (High Speed External) clock */
	RST_CLK_HSEconfig(RST_CLK_HSE_ON);    _ASSERT(RST_CLK_HSEstatus() != ERROR);
	
	/* Configures the CPU_PLL clock source */
	uint mul = MACS_INIT_CPU_FREQ_MHZ / 8;  // Предполагаем, что частота кварца 8 МГц.
	if ( mul ) 
		-- mul;
	if ( mul > RST_CLK_CPU_PLLmul16 )
		mul = RST_CLK_CPU_PLLmul16;	
	RST_CLK_CPU_PLLconfig(RST_CLK_CPU_PLLsrcHSEdiv1, mul);

	/* Enables the CPU_PLL */
	RST_CLK_CPU_PLLcmd(ENABLE);           _ASSERT(RST_CLK_CPU_PLLstatus() != ERROR);

	/* Enables the RST_CLK_PCLK_EEPROM */
	RST_CLK_PCLKcmd(RST_CLK_PCLK_EEPROM, ENABLE);
	/* Sets the code latency value */
//!!! move back 	EEPROM_SetLatency(EEPROM_Latency_5);

	/* Select the CPU_PLL output as input for CPU_C3_SEL */
	RST_CLK_CPU_PLLuse(ENABLE);
	/* Set CPUClk Prescaler */
	RST_CLK_CPUclkPrescaler(RST_CLK_CPUclkDIV1);

	/* Select the CPU clock source */
	RST_CLK_CPUclkSelection(RST_CLK_CPUclkCPU_C3);
	
	/* Enables the RTCHSE clock on all ports */
	RST_CLK_PCLKcmd(ALL_PORTS_CLK, ENABLE);

	MDR_RST_CLK->PER_CLOCK = 0xFFFFFFFF;
//!!!	Demo_Init();
#endif  // #ifndef MACS_CUSTOM_SYS_INIT  	
}

void System::HardFaultHandler()
{}
