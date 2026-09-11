// Globalize the inputs (from ps2 or usbs)

#include "layouts/kb_layouts.h"
#include <stdint.h>

/*
    actual_input:
    0 = from ps2 keyboard
    1 = from usb keyboard
*/

volatile scancode_t last_scancode;
volatile uint8_t actual_input = 0;