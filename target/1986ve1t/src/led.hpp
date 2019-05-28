#pragma once

#include "macs_system.hpp"

static const uint16_t led_tbl[] = {
	PORT_Pin_7,
	PORT_Pin_8,
	PORT_Pin_9,
	PORT_Pin_10,
	PORT_Pin_11,
	PORT_Pin_12,
	PORT_Pin_13,
	PORT_Pin_14};

#define NUM_LED  countof(led_tbl)

#define PCLK_LED  RST_CLK_PCLK_PORTD
#define PORT_LED  MDR_PORTD

class LedDriver
{
public:

	LedDriver()
	{
		RST_CLK_PCLKcmd(PCLK_LED, ENABLE);

		PORT_InitTypeDef init_struct;
		PORT_StructInit(&init_struct);
		init_struct.PORT_Pin = 0;
		for (uint i = 0; i < NUM_LED; ++i) {
			init_struct.PORT_Pin |= led_tbl[i];
		}
		init_struct.PORT_OE = PORT_OE_OUT;
		init_struct.PORT_SPEED = PORT_SPEED_FAST;
		init_struct.PORT_MODE = PORT_MODE_DIGITAL;

		PORT_Init(PORT_LED, &init_struct);
	}

	void Toggle(uint16_t i)
	{
		if (i < NUM_LED)
			PORT_WriteBit(PORT_LED, led_tbl[i], (BitAction)!PORT_ReadInputDataBit(PORT_LED, led_tbl[i]));
	}

	uint16_t GetNum() const
	{
		return NUM_LED;
	}

};
