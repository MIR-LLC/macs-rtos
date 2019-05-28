/// @file macs_critical_section.hpp
/// @brief Критические секции.
/// @details Классы для работы с критическими секциями. 
/// @copyright AstroSoft Ltd, 2016
  

#pragma once

#include <stdint.h>

#include "macs_system.hpp"
#include "macs_application.hpp"

namespace macs {

/// @brief Класс для представления критической секции.
/// @details Данный класс предназначен для выполнения участков кода, которые не должны быть
/// прерваны переключением контекста, т.е. атомарно, тем самым, устраняя условие гонки.
/// При создании объекта CriticalSection выполняется вход в критическую секцию путём маскирования
/// (отключения) прерываний (см. описание метода SystemBase::DisableIrq()). 
/// При уничтожении устанавливается маска прерываний, которая была до создания данного объекта,
/// что позволяет делать вложенные критические секции, и, соответственно, только при уничтожении объекта 
/// самого верхнего уровня произойдёт выход из критической секции, и прерывания снова будут включены.
class CriticalSection 
{
public:
	inline CriticalSection() {
		if ( ! System::IsInPrivOrIrq() )
			MACS_ALARM(AR_NOT_IN_PRIVILEGED);

		m_prev_interrupt_mask = System::DisableIrq();
	}
	inline ~CriticalSection() {
		System::EnableIrq(m_prev_interrupt_mask);
	}

private:
	CLS_COPY(CriticalSection)
private:
	uint32_t m_prev_interrupt_mask;
};

}	// namespace macs

