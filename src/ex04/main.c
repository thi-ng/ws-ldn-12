#include "stm32746g_discovery_audio.h"

#include "ct-head/math.h"
#include "ct-head/random.h"

#include "common/clockconfig.h"

#define VOLUME 50
#define SAMPLE_RATE 44100

// in bytes...
#define AUDIO_DMA_BUFFER_SIZE 512
// 16bit words
#define AUDIO_DMA_BUFFER_SIZE2 (AUDIO_DMA_BUFFER_SIZE >> 1)
// half buffer size (in 16bit words)
#define AUDIO_DMA_BUFFER_SIZE4 (AUDIO_DMA_BUFFER_SIZE >> 2)
// number of stereo samples (16bit words)
#define AUDIO_DMA_BUFFER_SIZE8 (AUDIO_DMA_BUFFER_SIZE >> 3)

#define TAU_RATE (CT_TAU / (float)SAMPLE_RATE)
#define HZ_TO_RAD(freq) ((freq)*TAU_RATE)

typedef struct {
  float phase;
  float freq;
  float mod_phase;
  float mod_freq;
  float mod_amp;
  uint8_t type;
  uint8_t mod_type;
} Oscillator;

extern SAI_HandleTypeDef haudio_out_sai;
extern void Error_Handler();

static uint8_t audioBuf[AUDIO_DMA_BUFFER_SIZE];

static CT_XorShift rnd;

// clang-format off
static Oscillator osc = {
  .phase     = 0.0f,
  .freq      = HZ_TO_RAD(220.0f),
  .type      = 0,
  .mod_phase = 0.0f,
  .mod_freq  = HZ_TO_RAD(0.5f),
  .mod_amp   = HZ_TO_RAD(110.0f),
  .mod_type  = 2
};
// clang-format on

int main() {
  CPU_CACHE_Enable();
  HAL_Init();
  SystemClock_Config();
  ct_xors_init(&rnd);

  if (BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, VOLUME, SAMPLE_RATE) != 0) {
    Error_Handler();
  }
  BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);
  BSP_AUDIO_OUT_SetVolume(VOLUME);
  BSP_AUDIO_OUT_Play((uint16_t *)audioBuf, AUDIO_DMA_BUFFER_SIZE);

  while (1) {
  }

  return 0;
}

static float compute_osc(size_t type, float phase) {
  switch (type) {
    case 0:
      return sinf(phase);
    case 1:  // saw
      return ct_mapf(phase, 0.0f, CT_TAU, 1.f, -1.f);
    case 2:  // square
      return phase < CT_PI ? -1.0f : 1.0f;
    case 3:  // triangle
      if (phase < CT_PI) {
        return ct_mapf(phase, 0.0f, CT_PI, -1.f, 1.f);
      } else {
        return ct_mapf(phase, CT_PI, CT_TAU, 1.f, -1.f);
      }
    case 4:  // saw + sin
      if (phase < CT_PI) {
        return ct_mapf(phase, 0.0f, CT_PI, -1.f, 1.f);
      } else {
        return sinf(phase);
      }
    case 5:
      return ct_xors_normpos(&rnd);
  }
  return 0;
}

static void renderAudio(int16_t *ptr) {
  size_t len = AUDIO_DMA_BUFFER_SIZE8;
  int16_t y;
  float f;
  while (len--) {
    osc.mod_phase += osc.mod_freq;
    if (osc.mod_phase >= CT_TAU) {
      osc.mod_phase -= CT_TAU;
    }
    f = osc.freq + osc.mod_amp * compute_osc(osc.mod_type, osc.mod_phase);
    osc.phase = ct_wrapf(osc.phase + f, CT_TAU);
    y         = ct_clamp16(compute_osc(osc.type, osc.phase) * 32767.f);
    *ptr++    = y;
    *ptr++    = y;
  }
}

void AUDIO_OUT_SAIx_DMAx_IRQHandler(void) {
  HAL_DMA_IRQHandler(haudio_out_sai.hdmatx);
}

void BSP_AUDIO_OUT_HalfTransfer_CallBack(void) {
  renderAudio((int16_t *)&audioBuf[0]);
}

void BSP_AUDIO_OUT_TransferComplete_CallBack(void) {
  renderAudio((int16_t *)&audioBuf[0] + AUDIO_DMA_BUFFER_SIZE4);
}

void BSP_AUDIO_OUT_Error_CallBack(void) {
  Error_Handler();
}
