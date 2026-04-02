#pragma once

#include <stdint.h>

uint8_t prospector_brightness_get(void);
int prospector_brightness_set(uint8_t brightness);
void prospector_brightness_clear_override(void);
