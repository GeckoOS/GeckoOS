#include "drivers/vga.h"
#include <stdint.h>
#define NANOPRINTF_IMPLEMENTATION
#include "terminal/printf.h"
#include "terminal/terminal.h"
#include "stdarg.h"


uint8_t printf_color = VGA_COLOR_WHITE;
static void wrapper(int c, void *ctx) {
  (void)ctx;
  putchar(c, printf_color);
}

void set_printf_color(uint8_t color) { printf_color = color; }

int printf(const char *fmt, ...) {
  va_list a;
  va_start(a, fmt);
  int r = npf_vpprintf(wrapper, NULL, fmt, a);
  va_end(a);
  return r;
}

int snprintf(char *restrict buf, size_t siz, const char *restrict fmt, ...) {
  va_list a;
  va_start(a, fmt);
  int r = npf_vsnprintf(buf, siz, fmt, a);
  va_end(a);
  return r;
}