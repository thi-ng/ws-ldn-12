catalog = make/sources.txt

USER_INCLUDES += -Iext/ct-synstack/src -Isrc/synth_extras
USER_SRC += $(wildcard ext/ct-synstack/src/*.c)
USER_SRC += src/synth_extras/osc_noise.c src/synth_extras/sdram_delay.c

CFLAGS += -O3 -ffast-math
