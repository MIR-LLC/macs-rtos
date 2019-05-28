#pragma once

#include <stdint.h>
#include "macs_tunes.h"
#include "stm32f4xx_hal.h"

#define LED0  (1<<13)
#define LED1  (1<<14)

const uint16_t led_tbl[] = {
	LED0,
	LED1
};

#define NUM_LED 2

class LedDriver
{
public:

	LedDriver()
	{
		__GPIOG_CLK_ENABLE();

		GPIO_InitTypeDef GPIO_InitStruct;
		GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
		HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
	}

	void Toggle(uint16_t i)
	{
		if (i >= NUM_LED)
			return;
		HAL_GPIO_TogglePin(GPIOG, led_tbl[i]);
	}

	uint16_t GetNum() const
	{
		return NUM_LED;
	}

};
