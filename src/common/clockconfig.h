#pragma once

#include "stm32f7xx_hal.h"

void SystemClock_Config(void);
void SystemClock_Config_USB(void);
void CPU_CACHE_Enable(void);

extern void Error_Handler();
