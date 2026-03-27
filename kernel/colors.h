// Added by MorganPG1 to centralise colors
#ifndef COLORS_H
#define COLORS_H

#include "drivers/vga.h"
#include <stdint.h>

#define BG_COLOR        VGA_COLOR_BLACK
#define TERM_COLOR      VGA_COLOR_WHITE | BG_COLOR << 4
#define PROMPT_COLOR    VGA_COLOR_LIGHT_GREEN | BG_COLOR << 4
#define BOLD_COLOR      VGA_COLOR_LIGHT_GREY | BG_COLOR << 4

#endif
