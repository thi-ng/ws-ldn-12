#include "stm32746g_discovery.h"
#include "stm32f7xx_hal.h"

#include "common/clockconfig.h"

#define TIMx TIM3
#define TIMx_CLK_ENABLE() __HAL_RCC_TIM3_CLK_ENABLE()
#define TIMx_IRQn TIM3_IRQn
#define TIMx_IRQHandler TIM3_IRQHandler

TIM_HandleTypeDef timer;
static uint8_t isBlinking = 1;

extern void Error_Handler();
static void initTimer(uint16_t period);

int main(void) {
  CPU_CACHE_Enable();
  HAL_Init();
  SystemClock_Config();
  BSP_LED_Init(LED_GREEN);
  BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);

  // Start timer w/ 500ms interval (time base = 10kHz)
  initTimer(5000);

  while (1)
    ;

  return 0;
}

// Initialize timer w/ 10kHz resolution and given period
static void initTimer(uint16_t period) {
  uint32_t prescaler           = (uint32_t)((SystemCoreClock / 2) / 10000) - 1;
  timer.Instance               = TIMx;
  timer.Init.Period            = period - 1;
  timer.Init.Prescaler         = prescaler;
  timer.Init.ClockDivision     = 0;
  timer.Init.CounterMode       = TIM_COUNTERMODE_UP;
  timer.Init.RepetitionCounter = 0;

  if (HAL_TIM_Base_Init(&timer) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_TIM_Base_Start_IT(&timer) != HAL_OK) {
    Error_Handler();
  }
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim) {
  // TIMx Peripheral clock enable
  TIMx_CLK_ENABLE();
  // Set the TIMx priority
  HAL_NVIC_SetPriority(TIMx_IRQn, 3, 0);
  // Enable the TIMx global Interrupt
  HAL_NVIC_EnableIRQ(TIMx_IRQn);
}

// Timer interrupt request.
void TIMx_IRQHandler(void) {
  HAL_TIM_IRQHandler(&timer);
}

// Callback function run whenever timer caused interrupt
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (isBlinking) {
    BSP_LED_Toggle(LED_GREEN);
  }
}

// External line 0 interrupt request.
void EXTI15_10_IRQHandler(void) {
  HAL_GPIO_EXTI_IRQHandler(KEY_BUTTON_PIN);
}

// Callback function run whenever user button has been pressed
void HAL_GPIO_EXTI_Callback(uint16_t pin) {
  if (pin == KEY_BUTTON_PIN) {
    isBlinking = !isBlinking;
  }
}
