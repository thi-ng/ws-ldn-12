#include "sdram_delay.h"
#include "delay.h"

static float delay_cache_r[AUDIO_BUFFER_SIZE * 2];
static float delay_cache_w[AUDIO_BUFFER_SIZE * 2];

CTSS_DSPNode *ctss_delay_sdram(char *id,
                               CTSS_DSPNode *src,
                               uint32_t len,
                               float feedback,
                               uint8_t channels) {
  CTSS_DSPNode *node     = ctss_node(id, channels);
  CTSS_DelayState *state = malloc(sizeof(CTSS_DelayState));
  state->delayLine       = ct_sdram_calloc(len * channels, sizeof(float));
  state->src             = src->buf;
  state->len             = len * channels;
  state->feedback        = feedback;
  state->channels        = channels;
  state->writePos        = 0;
  state->readPos         = channels;
  state->writePtr        = state->delayLine;
  state->readPtr         = state->delayLine + channels;
  node->state            = state;
  node->handler          = ctss_process_delay_sdram;
  return node;
}

uint8_t ctss_process_delay_sdram(CTSS_DSPNode *node,
                                 CTSS_DSPStack *stack,
                                 CTSS_Synth *synth) {
  CTSS_UNUSED(synth);
  CTSS_UNUSED(stack);
  CTSS_DelayState *state = node->state;
  float fb               = state->feedback;
  uint32_t read          = 0;
  uint32_t write         = 0;
  uint32_t raddr         = (uint32_t)state->delayLine + state->readPos * 4;
  uint32_t waddr         = (uint32_t)state->delayLine + state->writePos * 4;
  float *src             = state->src;
  float *buf             = node->buf;
  size_t len             = AUDIO_BUFFER_SIZE * state->channels;
  BSP_SDRAM_ReadData(waddr, (uint32_t *)delay_cache_w, len);
  BSP_SDRAM_ReadData(raddr, (uint32_t *)delay_cache_r, len);
  while (len--) {
    delay_cache_w[write] = (*src++) + delay_cache_r[read++] * fb;
    *buf++               = delay_cache_w[write++];
    if (++state->readPos == state->len) {
      state->readPos = 0;
      read           = 0;
      raddr          = (uint32_t)state->delayLine;
      BSP_SDRAM_ReadData(raddr, (uint32_t *)delay_cache_r, len);
    }
    if (++state->writePos == state->len) {
      BSP_SDRAM_WriteData(waddr, (uint32_t *)delay_cache_w, write);
      waddr           = (uint32_t)state->delayLine;
      state->writePos = 0;
      write           = 0;
      BSP_SDRAM_ReadData(waddr, (uint32_t *)delay_cache_w, len);
    }
  }
  BSP_SDRAM_WriteData(waddr, (uint32_t *)delay_cache_w, write);
  return 0;
}
