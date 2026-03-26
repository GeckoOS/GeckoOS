// bonk enjoyer (dorito girl)

// Small header for interacting with ports

#include <stdint.h>

#define PAUSE() asm volatile("pause")
#define CLI() asm volatile("cli")
#define STI() asm volatile("sti")
#define HALT() asm volatile("hlr")

void outb(uint16_t port, uint8_t val);
void outw(uint16_t port, uint16_t val);
uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
