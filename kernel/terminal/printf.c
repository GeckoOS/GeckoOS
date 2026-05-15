#define NANOPRINTF_IMPLEMENTATION
#include "terminal/printf.h"
#include "terminal/terminal.h"
#include "stdarg.h"


static void wrapper(int c, void *ctx) {
  (void)ctx;
  putchar(c, 0x0f);
}

int printkf(const char *fmt, ...) {
  va_list a;
  va_start(a, fmt);
  int r = npf_vpprintf(wrapper, NULL, fmt, a);
  va_end(a);
  return r;
}

int snprintkf(char *restrict buf, size_t siz, const char *restrict fmt, ...) {
  va_list a;
  va_start(a, fmt);
  int r = npf_vsnprintf(buf, siz, fmt, a);
  va_end(a);
  return r;
}