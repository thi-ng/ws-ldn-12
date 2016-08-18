#include "stm32746g_discovery.h"
#include "stm32f7xx_hal.h"

void Error_Handler(void) {
  BSP_LED_On(LED_GREEN);
  while (1) {
  }
}

void SysTick_Handler(void) {
  HAL_IncTick();
}
