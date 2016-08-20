#include "osc_noise.h"
#include "ct-head/random.h"

static CT_XorShift rnd;

void ctss_osc_noise_init() {
  ct_xors_init(&rnd);
}

uint8_t ctss_process_osc_noise(CTSS_DSPNode *node,
                               CTSS_DSPStack *stack,
                               CTSS_Synth *synth) {
  CTSS_UNUSED(synth);
  CTSS_UNUSED(stack);
  CTSS_OscState *state = node->state;
  float *buf           = node->buf;
  size_t len           = AUDIO_BUFFER_SIZE;
  while (len--) {
    *buf++ = (state->gain * ct_xors_norm(&rnd)) + state->dcOffset;
  }
  return 0;
}
