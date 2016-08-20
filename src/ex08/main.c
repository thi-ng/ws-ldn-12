#include "stm32746g_discovery.h"
#include "stm32746g_discovery_audio.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32f7xx_hal.h"

#include "common/clockconfig.h"
#include "usbh_MIDI.h"

#include <stdio.h>
#include <string.h>

#define VOLUME 70

#define AUDIO_DMA_BUFFER_SIZE 512
#define AUDIO_DMA_BUFFER_SIZE2 (AUDIO_DMA_BUFFER_SIZE >> 1)

typedef enum {
  BUFFER_OFFSET_NONE = 0,
  BUFFER_OFFSET_HALF,
  BUFFER_OFFSET_FULL
} DMABufferState;

extern HCD_HandleTypeDef hhcd;
extern USBH_HandleTypeDef hUSBH;
extern SAI_HandleTypeDef haudio_out_sai;

static uint8_t audio_buf[AUDIO_DMA_BUFFER_SIZE] = {0};
static uint8_t midi_receive_buf[USB_MIDI_RX_BUFFER_SIZE];

static DMABufferState dma_state = BUFFER_OFFSET_NONE;

static void process_usbh_message(USBH_HandleTypeDef *host, uint8_t msg);
static void start_playback();
static void stop_playback();

static uint16_t curr_line = 0;
static char line_buf[64];

int main() {
  CPU_CACHE_Enable();
  HAL_Init();
  SystemClock_Config();
  BSP_LED_Init(LED_GREEN);
  BSP_LCD_Init();
  BSP_LCD_LayerDefaultInit(LTDC_ACTIVE_LAYER, SDRAM_DEVICE_ADDR);
  BSP_LCD_SelectLayer(LTDC_ACTIVE_LAYER);
  BSP_LCD_SetFont(&Font12);
  BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
  BSP_LCD_Clear(LCD_COLOR_BLUE);
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);

  USBH_Init(&hUSBH, process_usbh_message, 0);
  USBH_RegisterClass(&hUSBH, USBH_MIDI_CLASS);
  USBH_Start(&hUSBH);
  while (1) {
    USBH_Process(&hUSBH);
  }
  return 0;
}

static void log_to_screen(const char *buf) {
  BSP_LCD_ClearStringLine(curr_line + 1);
  BSP_LCD_DisplayStringAtLine(curr_line, (uint8_t *)buf);
  curr_line = (curr_line + 1) % 20;
}

static void process_usbh_message(USBH_HandleTypeDef *host, uint8_t msg) {
  switch (msg) {
    case HOST_USER_SELECT_CONFIGURATION:
      break;

    case HOST_USER_DISCONNECTION:
      stop_playback();
      break;

    case HOST_USER_CLASS_ACTIVE:
      USBH_MIDI_Receive(&hUSBH, midi_receive_buf, USB_MIDI_RX_BUFFER_SIZE);
      start_playback();
      break;

    case HOST_USER_CONNECTION:
      break;

    default:
      break;
  }
}

static void start_playback() {
  BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, VOLUME, 44100);
  BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);
  BSP_AUDIO_OUT_Play((uint16_t *)audio_buf, AUDIO_DMA_BUFFER_SIZE);
  BSP_LED_On(LED_GREEN);
}

static void stop_playback() {
  BSP_LED_Off(LED_GREEN);
}

static char *midi_msg_types[] = {"OFF", "ON", "PP", "CC",
                                 "PC",  "AT", "PB", "SYSEX"};

static void process_midi_packets() {
  midi_package_t *packet = (midi_package_t *)midi_receive_buf;
  uint16_t num           = USBH_MIDI_GetLastReceivedDataSize(&hUSBH) / 4;
  if (num > 0) {
    while (num--) {
      if (packet->type > 7) {
        snprintf(line_buf, 64, "%s: c:%u e:%u v:%u/%u   ",
                 midi_msg_types[packet->type - 8], packet->chn, packet->event,
                 packet->value1, packet->value2);
        log_to_screen(line_buf);
      }
      packet++;
    }
  }
}

void OTG_FS_IRQHandler(void) {
  HAL_HCD_IRQHandler(&hhcd);
}

void USBH_MIDI_ReceiveCallback(USBH_HandleTypeDef *phost) {
  BSP_LED_Toggle(LED_GREEN);
  process_midi_packets();
  USBH_MIDI_Receive(&hUSBH, midi_receive_buf, USB_MIDI_RX_BUFFER_SIZE);
  //canReceive = 1;
}

void AUDIO_OUT_SAIx_DMAx_IRQHandler(void) {
  HAL_DMA_IRQHandler(haudio_out_sai.hdmatx);
}
