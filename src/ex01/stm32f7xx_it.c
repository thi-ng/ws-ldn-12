#include "stm32f7xx_it.h"

void Error_Handler(void) {
  BSP_LED_On(LED_GREEN);
  while (1) {
  }
}

void SysTick_Handler(void) {
  HAL_IncTick();
}

void EXTI15_10_IRQHandler(void) {
  HAL_GPIO_EXTI_IRQHandler(KEY_BUTTON_PIN);
}
