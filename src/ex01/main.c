#include "stm32746g_discovery.h"
#include "stm32f7xx_hal.h"

#include "common/clockconfig.h"

#define LED_SPEED 100

volatile int do_blink = 1;

int main() {
  HAL_Init();
  SystemClock_Config();

  BSP_LED_Init(LED_GREEN);

  // configure user button in ISR mode
  BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);

  while (1) {
    if (do_blink) {
      BSP_LED_On(LED_GREEN);
      HAL_Delay(LED_SPEED);
      BSP_LED_Off(LED_GREEN);
      HAL_Delay(LED_SPEED);
    }
  }
  return 0;
}

void EXTI15_10_IRQHandler(void) {
  HAL_GPIO_EXTI_IRQHandler(KEY_BUTTON_PIN);
}

void HAL_GPIO_EXTI_Callback(uint16_t pin) {
  if (pin == KEY_BUTTON_PIN) {
    while (BSP_PB_GetState(BUTTON_KEY) != RESET)
      ;
    do_blink = !do_blink;
  }
}
