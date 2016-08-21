#include <stdio.h>

#include "stm32746g_discovery_lcd.h"
#include "usbh_MIDI.h"

void init_midi_log();
void log_to_lcd(const char* msg);
void log_midi_packet(const midi_package_t* packet);
