/// @file macs_system_cfg.h
/// @brief Параметры компиляции ОС, относящиеся к расширениям, аппаратуре, etc.

#pragma once

#define MACS_USE_MPU             1
#define MACS_MPU_PROTECT_NULL    1     ///< Защита памяти по нулевому адресу от доступа.
#define MACS_MPU_PROTECT_STACK   1     ///< Аппаратная защита стека от переполнения.
