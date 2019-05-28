/// @file macs_system.cpp
/// @brief Конфигурация аппаратной части.
/// @details Класс System имеет специфичную для STM32F4-Discovery реализацию.
/// @copyright AstroSoft Ltd, 2016

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"

#include "macs_system.hpp"

/* System Clock Configuration */
void System::InitClock()
{
	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;

	/* Enable Power Control clock */
	__PWR_CLK_ENABLE()
	;

	/* The voltage scaling allows optimizing the power consumption when the
	 device is clocked below the maximum system frequency (see datasheet). */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/* Enable HSE Oscillator and activate PLL with HSE as source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = MACS_INIT_CPU_FREQ_MHZ * RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	HAL_RCC_OscConfig(&RCC_OscInitStruct);

	/* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
	HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

bool System::SetUpIrqHandling(int irq_num, bool vector, bool enable)
{
	bool result = false;
	if ((irq_num >= 0) && (irq_num <= DMA2D_IRQn)) {
		result = true;
		if (enable) {
			NVIC_EnableIRQ((IRQn_Type)irq_num);
		} else {
			NVIC_DisableIRQ((IRQn_Type)irq_num);
		}
	}

	return result;
}

void System::RaiseIrq(int irq_num)
{
	NVIC_SetPendingIRQ((IRQn_Type)irq_num);
}

extern void Configure_PC13(void);

void System::InitCpu()
{
	HAL_Init();
	InitClock();
}

void System::HardFaultHandler()
{
	static uint32_t icsr = SCB->ICSR;
	static uint32_t cfsr = SCB->CFSR;
	static uint32_t hfsr = SCB->HFSR;
	static uint32_t dfsr = SCB->DFSR;
	static uint32_t afsr = SCB->AFSR;
	static uint32_t mmfar = SCB->MMFAR;
	static uint32_t bfar = SCB->BFAR;
	static uint32_t shcsr = SCB->SHCSR;
}

