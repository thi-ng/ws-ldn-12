catalog = make/sources.txt

USER_INCLUDES += -Iext/ct-synstack/src
USER_SRC += $(wildcard ext/ct-synstack/src/*.c)

CFLAGS += -O3 -ffast-math
