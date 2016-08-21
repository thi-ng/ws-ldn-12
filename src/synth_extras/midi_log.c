#include "midi_log.h"

#ifdef LOG_MIDI

static char *midi_msg_types[] = {"OFF", "ON", "PP", "CC",
                                 "PC",  "AT", "PB", "SYSEX"};

void init_midi_log() {
  BSP_LCD_Init();
  BSP_LCD_LayerDefaultInit(LTDC_ACTIVE_LAYER, SDRAM_DEVICE_ADDR);
  BSP_LCD_SelectLayer(LTDC_ACTIVE_LAYER);
  BSP_LCD_SetFont(&Font12);
  BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
  BSP_LCD_Clear(LCD_COLOR_BLUE);
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
}

void log_to_lcd(const char *msg) {
  static uint16_t curr_line = 0;
  BSP_LCD_ClearStringLine(curr_line + 1);
  BSP_LCD_DisplayStringAtLine(curr_line, (uint8_t *)msg);
  curr_line = (curr_line + 1) % 20;
}

void log_midi_packet(const midi_package_t *packet) {
  static char buf[64];
  snprintf(buf, 64, "%s: c:%u e:%u v:%u/%u   ",
           midi_msg_types[packet->type - 8], packet->chn, packet->event,
           packet->value1, packet->value2);
  log_to_lcd(buf);
}

#else

void init_midi_log() {
}
void log_to_lcd(const char* msg) {
}
void log_midi_packet(const midi_package_t* packet) {
}

#endif
