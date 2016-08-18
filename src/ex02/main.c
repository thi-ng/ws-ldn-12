#include "stm32f7xx_hal.h"

#include "ct-gui/gui_stm32.h"
#include "ct-head/random.h"

#include "common/clockconfig.h"

static CTGUI gui;
static CT_Smush rnd;

static void demoWelcome();

int main() {
  CPU_CACHE_Enable();
  HAL_Init();
  SystemClock_Config();

  ct_smush_init(&rnd, 0xdecafbad);

  BSP_LCD_Init();

  if (BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize()) == TS_OK) {
    BSP_LCD_LayerDefaultInit(LTDC_ACTIVE_LAYER, SDRAM_DEVICE_ADDR);
    BSP_LCD_SelectLayer(LTDC_ACTIVE_LAYER);

    demoWelcome();
  }
  return 0;
}

static void demoWelcome() {
  BSP_LCD_SetFont(&CTGUI_FONT);
  BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
  BSP_LCD_Clear(LCD_COLOR_WHITE);
  BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
  BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2 - 8,
                          (uint8_t *)"STM32F746G", CENTER_MODE);
  BSP_LCD_SetTextColor(LCD_COLOR_DARKBLUE);
  BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2 + 8, (uint8_t *)"Welcome!",
                          CENTER_MODE);
  const float w = BSP_LCD_GetXSize() - 5;
  const float h = BSP_LCD_GetYSize() - 5;
  while (1) {
    BSP_LCD_SetTextColor((ct_smush(&rnd) & 0xffffff) | 0xff000000);
    BSP_LCD_FillCircle(ct_smush_minmax(&rnd, 5.f, w),
                       ct_smush_minmax(&rnd, 5.f, h), 4);
    HAL_Delay(1);
  }
}

void Error_Handler(void) {
  BSP_LED_On(LED_GREEN);
  while (1) {
  }
}

void SysTick_Handler(void) {
  HAL_IncTick();
}

void EXTI15_10_IRQHandler(void) {
  // Interrupt handler shared between:
  // SD_DETECT pin, USER_KEY button and touch screen interrupt
  if (__HAL_GPIO_EXTI_GET_IT(SD_DETECT_PIN) != RESET) {
    HAL_GPIO_EXTI_IRQHandler(SD_DETECT_PIN | TS_INT_PIN);
  } else {
    // User button event or Touch screen interrupt
    HAL_GPIO_EXTI_IRQHandler(KEY_BUTTON_PIN);
  }
}
