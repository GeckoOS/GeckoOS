#ifndef PRINTF_H
#define PRINTF_H

#include "drivers/vga.h"
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS   1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS       1
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS       1
#define NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS       1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS      1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS   0

#include <stdint.h>

#include "terminal/npf.h"
#include "terminal/terminal.h"

extern uint8_t printkf_text_color;

int printkf(const char *fmt, ...);
int snprintkf(char *buf, size_t siz, const char *fmt, ...);
void set_printkf_color(enum VGA_COLOR color, bool restart /* Restart the printkf text color to the previus color after one use */);

static inline void nomacro_print(const char* what, uint8_t color) { print("["); printc(what, color); print("] "); }

#define EPRINT(...) nomacro_print("FAILED", VGA_COLOR_RED); printkf(__VA_ARGS__) // Prints error
#define WPRINT(...) nomacro_print(" WARN ", VGA_COLOR_LIGHT_YELLOW); printkf(__VA_ARGS__) // Prints warning
#define IPRINT(...) nomacro_print(" INFO ", VGA_COLOR_LIGHT_GREY); set_printkf_color(VGA_COLOR_LIGHT_GREY, 1); printkf(__VA_ARGS__) // Prints info
#define OPRINT(...) nomacro_print("  OK  ", VGA_COLOR_GREEN); printkf(__VA_ARGS__) // Prints success

#endif
