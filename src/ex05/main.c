#include "stm32746g_discovery.h"
#include "stm32746g_discovery_audio.h"
#include "stm32f7xx_hal.h"

#include "ff.h"
#include "ff_gen_drv.h"
#include "usbh_diskio.h"

#include "common/clockconfig.h"

#define VOLUME 70

#define AUDIO_DMA_BUFFER_SIZE 4096
#define AUDIO_DMA_BUFFER_SIZE2 (AUDIO_DMA_BUFFER_SIZE >> 1)

#define WAVEFILENAME "0:sound.wav"

typedef struct {
  uint32_t riffTag;
  uint32_t riffLength;
  uint32_t waveTag;
  uint32_t formatTag;
  uint32_t formatLength;
  uint16_t audioFormat;
  uint16_t numChannels;
  uint32_t sampleRate;
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bits;
  uint32_t dataTag;
  uint32_t dataLength;
} WavHeader;

typedef enum {
  BUFFER_OFFSET_NONE = 0,
  BUFFER_OFFSET_HALF,
  BUFFER_OFFSET_FULL
} DMABufferState;

extern HCD_HandleTypeDef hhcd;
extern USBH_HandleTypeDef hUSBH;
extern SAI_HandleTypeDef haudio_out_sai;

static uint8_t audio_buf[AUDIO_DMA_BUFFER_SIZE];
static DMABufferState dma_state = BUFFER_OFFSET_NONE;

FIL audio_file;
WavHeader audio_format;
UINT bytes_read;

static char usb_drive_path[4];

static void process_usbh_message(USBH_HandleTypeDef *host, uint8_t msg);
static void start_audio_app();
static void start_playback();
static void stop_playback();
static void play_wav_file();

int main() {
  CPU_CACHE_Enable();
  HAL_Init();
  SystemClock_Config();
  BSP_LED_Init(LED_GREEN);
  if (FATFS_LinkDriver(&USBH_Driver, usb_drive_path) == 0) {
    USBH_Init(&hUSBH, process_usbh_message, 0);
    USBH_RegisterClass(&hUSBH, USBH_MSC_CLASS);
    USBH_Start(&hUSBH);
    while (1) {
      USBH_Process(&hUSBH);
    }
  } else {
    Error_Handler();
  }
  return 0;
}

static void process_usbh_message(USBH_HandleTypeDef *host, uint8_t msg) {
  switch (msg) {
    case HOST_USER_SELECT_CONFIGURATION:
      break;

    case HOST_USER_DISCONNECTION:
      f_mount(NULL, (TCHAR const *)"", 0);
      stop_playback();
      break;

    case HOST_USER_CLASS_ACTIVE:
      start_playback();
      break;

    case HOST_USER_CONNECTION:
      break;

    default:
      break;
  }
}

static void start_playback() {
  if (f_open(&audio_file, WAVEFILENAME, FA_READ) == FR_OK) {
    f_read(&audio_file, &audio_format, sizeof(WavHeader), &bytes_read);
    play_wav_file();
  } else {
    Error_Handler();
  }
}

static void play_wav_file() {
  BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, VOLUME, audio_format.sampleRate);
  BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);
  f_lseek(&audio_file, 0);
  f_read(&audio_file, audio_buf, AUDIO_DMA_BUFFER_SIZE, &bytes_read);
  uint32_t samples_remaining = audio_format.riffLength - bytes_read;
  BSP_AUDIO_OUT_Play((uint16_t *)audio_buf, AUDIO_DMA_BUFFER_SIZE);
  uint32_t err = 0;
  while (!err) {
    bytes_read = 0;
    if (dma_state == BUFFER_OFFSET_HALF) {
      err = (f_read(&audio_file, audio_buf, AUDIO_DMA_BUFFER_SIZE2,
                    &bytes_read) != FR_OK);
      dma_state = BUFFER_OFFSET_NONE;
    } else if (dma_state == BUFFER_OFFSET_FULL) {
      err = (f_read(&audio_file, &audio_buf[AUDIO_DMA_BUFFER_SIZE2],
                    AUDIO_DMA_BUFFER_SIZE2, &bytes_read) != FR_OK);
      dma_state = BUFFER_OFFSET_NONE;
    }
    if (!err) {
      if (samples_remaining > AUDIO_DMA_BUFFER_SIZE2) {
        samples_remaining -= bytes_read;
      } else {
        f_lseek(&audio_file, 0);
        samples_remaining = audio_format.riffLength - bytes_read;
      }
      BSP_LED_Toggle(LED_GREEN);
    }
  }
  stop_playback();
}

static void stop_playback() {
  BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW);
}

void OTG_FS_IRQHandler(void) {
  HAL_HCD_IRQHandler(&hhcd);
}

void AUDIO_OUT_SAIx_DMAx_IRQHandler(void) {
  HAL_DMA_IRQHandler(haudio_out_sai.hdmatx);
}

void BSP_AUDIO_OUT_HalfTransfer_CallBack(void) {
  dma_state = BUFFER_OFFSET_HALF;
}

void BSP_AUDIO_OUT_TransferComplete_CallBack(void) {
  dma_state = BUFFER_OFFSET_FULL;
}

void BSP_AUDIO_OUT_Error_CallBack(void) {
  Error_Handler();
}
