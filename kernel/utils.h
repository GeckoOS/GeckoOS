// bonk enjoyer (dorito girl)

// Small header for interacting with ports

#include <stdint.h>

#define PAUSE() asm volatile("pause")
#define CLI() asm volatile("cli")
#define STI() asm volatile("sti")
#define HALT() asm volatile("hlr")
