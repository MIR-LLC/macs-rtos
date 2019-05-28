/// @file macs_system.hpp
/// @brief Конфигурация аппаратной части.
/// @details Определяет интерфейс доступа к аппаратно-зависимому функционалу.
/// Класс System должен иметь специфичную для используемого аппаратного обеспечения реализацию.
/// @copyright AstroSoft Ltd, 2016

#pragma once

#include "macs_common.hpp"
#include "macs_platform.hpp"
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"

#ifndef MACS_INIT_CPU_FREQ_MHZ
	#define MACS_INIT_CPU_FREQ_MHZ  168 
#endif	

#ifndef MACS_HEAP_SIZE
	#define MACS_HEAP_SIZE (32 KILO_B)
#endif	

/// @brief Класс, содержащий реализацию платформозависимых методов
class System: public SystemBase
{
public:
	static const uint32_t HEAP_SIZE = MACS_HEAP_SIZE;

	static void InitCpu();
	static void HardFaultHandler();

	static bool SetUpIrqHandling(int irq_num, bool vector, bool enable);

	/// @brief Возбуждение прерывания
	/// @details Вызов метода приводит к возникновению указанного прерывания. Прерывание должно быть предварительно разрешено
	/// @param irq_num - номер прерывания
	static void RaiseIrq(int irq_num);

private:
	static void InitClock();
};
