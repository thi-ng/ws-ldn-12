#pragma once

#include "common/sdram_alloc.h"
#include "synth.h"

CTSS_DSPNode *ctss_delay_sdram(char *id,
                               CTSS_DSPNode *src,
                               uint32_t len,
                               float feedback,
                               uint8_t channels);

uint8_t ctss_process_delay_sdram(CTSS_DSPNode *node,
                                 CTSS_DSPStack *stack,
                                 CTSS_Synth *synth);
