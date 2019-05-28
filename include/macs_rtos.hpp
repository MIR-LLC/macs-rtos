/// @file macs_rtos.hpp
/// @brief Подключение ядра системы.
/// @details Подключает все модули ядра системы.
/// @copyright AstroSoft Ltd, 2016

#include "macs_common.hpp"
#include "macs_critical_section.hpp"
#include "macs_scheduler.hpp"
#include "macs_event.hpp"
#include "macs_mutex.hpp"
#include "macs_task.hpp"
#include "macs_semaphore.hpp"
#include "macs_message_queue.hpp"
#include "macs_application.hpp"
#include "macs_profiler.hpp"

#if MACS_LIBRARY
extern "C" void MacsInit();
#endif
  
using namespace macs;
