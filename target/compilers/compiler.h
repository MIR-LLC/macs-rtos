#pragma once

#if defined ( __CC_ARM )
#include "arm\keil.h"
#elif defined ( __ICCARM__ )
#include "arm\iar.h"
#elif defined ( __GNUC__ )
#include "arm\gcc.h"
#else
#error Unknown compiler!
#endif
