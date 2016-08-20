#include "stm32746g_discovery_audio.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_sdram.h"

#include "adsr.h"
#include "biquad.h"
#include "foldback.h"
#include "node_ops.h"
#include "osc.h"
#include "osc_noise.h"
#include "panning.h"
#include "sdram_delay.h"
#include "synth.h"

#include "common/clockconfig.h"
#include "ct-head/random.h"

#define VOLUME 80
#define SAMPLE_RATE 44100

#define AUDIO_DMA_BUFFER_SIZE 512
#define AUDIO_DMA_BUFFER_SIZE2 (AUDIO_DMA_BUFFER_SIZE >> 1)
#define AUDIO_DMA_BUFFER_SIZE4 (AUDIO_DMA_BUFFER_SIZE >> 2)
#define AUDIO_DMA_BUFFER_SIZE8 (AUDIO_DMA_BUFFER_SIZE >> 3)

extern SAI_HandleTypeDef haudio_out_sai;
static uint8_t audioBuf[AUDIO_DMA_BUFFER_SIZE];

static CTSS_Synth synth;
static CT_Smush rnd;

static const uint8_t scale[]  = {36, 40, 43, 45, 55, 52, 48, 60};
static uint32_t note_id       = 0;
static uint32_t voice_id      = 0;
static __IO uint32_t new_note = 0;

static void init_voice(CTSS_Synth *synth, CTSS_DSPStack *stack);
static void trigger_note();
static void render_audio(int16_t *buf);

int main() {
  CPU_CACHE_Enable();
  HAL_Init();
  SystemClock_Config();
  BSP_LED_Init(LED_GREEN);

  // IMPORTANT! Initialize SDRAM before using sdram_malloc & co.
  if (BSP_SDRAM_Init() != SDRAM_OK) {
    Error_Handler();
  }

  ct_smush_init(&rnd, 0xcafebabe);
  ctss_osc_noise_init();
  ctss_init(&synth, 2);
  ctss_add_global_lfo(&synth, ctss_osc("lfo1", ctss_process_osc_sin, 0.0f,
                                       HZ_TO_RAD(0.125f), 0.4f, 0.8f));
  ctss_add_global_lfo(&synth, ctss_osc("lfo2", ctss_process_osc_sin, 0.0f,
                                       HZ_TO_RAD(0.25f), 0.495f, 0.5f));
  ctss_add_global_lfo(&synth, ctss_osc("lfo3", ctss_process_osc_tri, 0.0f,
                                       HZ_TO_RAD(0.1f), 0.499f, 0.5f));

  for (uint8_t i = 0; i < synth.numStacks; i++) {
    init_voice(&synth, &synth.stacks[i]);
  }
  ctss_collect_stacks(&synth);

  if (BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, VOLUME, SAMPLE_RATE) != 0) {
    Error_Handler();
  }
  BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);
  BSP_AUDIO_OUT_SetVolume(VOLUME);
  BSP_AUDIO_OUT_Play((uint16_t *)audioBuf, AUDIO_DMA_BUFFER_SIZE);

  while (1) {
    new_note = 1;
    BSP_LED_Toggle(LED_GREEN);
    HAL_Delay(750);
  }

  return 0;
}

// see README for DSP graph visualization

static void init_voice(CTSS_Synth *synth, CTSS_DSPStack *stack) {
  CTSS_DSPNode *env =
      ctss_adsr("env", synth->lfo[0], 0.01f, 0.05f, 0.85f, 1.0f, 0.25f);
  CTSS_DSPNode *osc1 = ctss_osc("osc1", ctss_process_osc_spiral, 0, 0, 0.3f, 0);
  CTSS_DSPNode *osc2 = ctss_osc("osc2", ctss_process_osc_sawsin, 0, 0, 0.3f, 0);
  CTSS_DSPNode *osc3 =
      ctss_osc("osc3", ctss_process_osc_noise, 0, 0, 0.025f, 0);
  CTSS_DSPNode *sum  = ctss_op4("sum", osc1, env, osc2, env, ctss_process_madd);
  CTSS_DSPNode *nsum = ctss_op2("nsum", osc3, synth->lfo[2], ctss_process_mult);
  CTSS_DSPNode *sum2 = ctss_op2("sum2", sum, nsum, ctss_process_sum);
  CTSS_DSPNode *fb   = ctss_foldback("fb", sum2, 0.1, 4.0f);
  CTSS_DSPNode *filter =
      ctss_filter_biquad("filter", LPF, fb, 1000.0f, 0.0f, 0.5f);
  CTSS_DSPNode *pan = ctss_panning("pan", filter, synth->lfo[1], 0.0f);
  CTSS_DSPNode *delay =
      ctss_delay_sdram("delay", pan, (uint32_t)(SAMPLE_RATE * 0.5f), 0.9f, 2);
  CTSS_DSPNode *nodes[] = {env,  osc1, osc2,   osc3, sum,  nsum,
                           sum2, fb,   filter, pan,  delay};
  ctss_build_stack(stack, nodes, 11);
}

static void trigger_note() {
  float time       = HAL_GetTick();
  int32_t pitch    = scale[ct_smush(&rnd) % 8] - (ct_smush(&rnd) & 1) * 12;
  float freq       = ctss_notes[pitch];
  freq             = HZ_TO_RAD(freq);
  CTSS_DSPStack *s = &synth.stacks[voice_id];
  ctss_reset_adsr(NODE_ID(s, "env"));
  NODE_ID_STATE(CTSS_OscState, s, "osc1")->freq = freq;
  NODE_ID_STATE(CTSS_OscState, s, "osc2")->freq = freq * 0.51;
  ctss_calculate_biquad_coeff(NODE_ID(s, "filter"), LPF,
                              2800.0f + sinf(time * 0.1f) * 2400.0f, -18.0f,
                              ct_smush_minmax(&rnd, 0.25f, 2.0f));
  ctss_activate_stack(s);
  note_id  = (note_id + 1) % 8;
  voice_id = (voice_id + 1) % synth.numStacks;
}

static void render_audio(int16_t *buf) {
  if (new_note) {
    trigger_note();
    new_note = 0;
  }
  ctss_update_mix_stereo_i16(&synth, ctss_mixdown_i16, AUDIO_DMA_BUFFER_SIZE8,
                             buf);
}

void AUDIO_OUT_SAIx_DMAx_IRQHandler(void) {
  HAL_DMA_IRQHandler(haudio_out_sai.hdmatx);
}

void BSP_AUDIO_OUT_HalfTransfer_CallBack(void) {
  render_audio((int16_t *)&audioBuf[0]);
}

void BSP_AUDIO_OUT_TransferComplete_CallBack(void) {
  render_audio((int16_t *)&audioBuf[AUDIO_DMA_BUFFER_SIZE2]);
}

void BSP_AUDIO_OUT_Error_CallBack(void) {
  Error_Handler();
}
