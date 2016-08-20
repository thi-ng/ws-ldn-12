catalog = make/sources.txt

USE_USBH  = 1
USBH_INCLUDES += -I$(EXT_SRC_DIR)/usbh_midi
USBH_SRC += $(EXT_SRC_DIR)/usbh_midi/usbh_MIDI.c

USER_INCLUDES += -Isrc/ex08 -Iext/ct-synstack/src

CFLAGS += -O3 -ffast-math
