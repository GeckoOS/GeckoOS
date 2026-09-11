#pragma once

#include "drivers/hid/keyboard.h"
#include "drivers/ps2keyboard.h"
#include "layouts/kb_layouts.h"
#include "terminal/printf.h"
#include <stdint.h>

extern volatile scancode_t last_scancode;
extern volatile uint8_t actual_input;

static inline uint8_t get_scancode() {
    if (actual_input) return hid_wfi();
    else return ps2_kb_wfi();
}