/// @file macs_tunes.h
/// @brief Параметры компиляции ОС, относящиеся к ядру.
/// @details Значения некоторых опций могут быть установлены в 1/0,
/// что соответствует включению/выключению возможностей операционной системы. Константа Build содержит весь набор в виде битовой маски.

#pragma once

#include "macs_config.hpp"
#include "macs_system_cfg.h"

#ifndef MACS_DEBUG
	#define MACS_DEBUG               0       ///< Режим отладки. Включаются внутренние проверки корректности работы.
#endif

#ifndef MACS_INIT_TICK_RATE_HZ
	#define MACS_INIT_TICK_RATE_HZ   1000u   ///< Начальная частота переключения задач в вытесняющем режиме.	Системный квант времени равен (1.0/MACS_INIT_TICK_RATE_HZ) секунд.
#endif
 
#ifndef MACS_MAX_STACK_SIZE
	#define MACS_MAX_STACK_SIZE      0x800u  ///< Максимальный размер стека задачи (в словах).
#endif

#ifndef MACS_MAIN_STACK_SIZE
	#define MACS_MAIN_STACK_SIZE     1000u   ///< Размер стека планировщика (в словах).
#endif

#ifndef MACS_WATCH_STACK
	#define MACS_WATCH_STACK         0	     ///< Режим автоматического отслеживания глубины использования стека.
#endif

#ifndef MACS_TASK_NAME_LENGTH
	#define MACS_TASK_NAME_LENGTH   -1       ///< Длина имени задачи.	При значении -1 имя хранится в динамической памяти. При значении 0 имена не используются.
#endif

#ifndef MACS_MAX_TASK_PRIORITY
	#define MACS_MAX_TASK_PRIORITY	PriorityRealtime  ///< Максимально возможный приоритет задач.	
#endif 
 
#ifndef MACS_MUTEX_PRIORITY_INVERSION		// решение проблемы инверсии приоритетов в мьютексах включено/выключено
	#define MACS_MUTEX_PRIORITY_INVERSION 1  ///< Режим инверсии приоритетов в мьтексах.	
#endif

#ifndef MACS_PROFILING_ENABLED
	#define MACS_PROFILING_ENABLED   0     ///< В процессе работы может измеряться время выполнения разных операций. Снижает производительность. 
#endif

#ifndef MACS_MEM_STATISTICS
	#define MACS_MEM_STATISTICS      0     ///< Распределитель памяти собирает статистику во время работы.
#endif
#ifndef MACS_MEM_WIPE
	#define MACS_MEM_WIPE            0     ///< Распределитель памяти затирает освобождаемую память мусором.
#endif
#ifndef MACS_MEM_ON_PAUSE
	#define MACS_MEM_ON_PAUSE        1     ///< При работе с динамической памятью используется механизм паузы планировщика вместо SpinLock.
#endif

#ifndef MACS_IRQ_FAST_SWITCH
	#define MACS_IRQ_FAST_SWITCH     1     ///< Переключение контекста после паузы происходит немедленно
#endif
 
#ifndef MACS_SLEEP_ON_IDLE
	#define MACS_SLEEP_ON_IDLE       0     ///< В задаче холостого хода процессор переводится в режим низкого энергопотребления.
#endif

#ifndef MACS_PRINTF_ALLOWED
	#define MACS_PRINTF_ALLOWED      0     ///< Разрешает использовать printf из ядра. В реальных приложениях может приводить к краху системы.
#endif

#ifndef MACS_AUTO_STACK_GROW
	#define MACS_AUTO_STACK_GROW     0     ///< При исчерпании стека происходит автоматическое увеличение его размера.
#endif

#ifndef MACS_WEAK_MAIN
	#define MACS_WEAK_MAIN           0     ///< Функция main объявлена как weak для возможности использования внешней.	
#endif

#ifndef MACS_LIBRARY
	#define MACS_LIBRARY             0     ///< ОС собирается как библиотека. Функция main должна быть определена в проекте.
#endif

#ifndef MACS_USE_CLOCK
	#define MACS_USE_CLOCK         	 0  ///< Использование надежных меток времени. 
#endif

#ifndef MACS_USE_LOG
	#define MACS_USE_LOG             0  ///< Использование журнала событий. 
#endif
