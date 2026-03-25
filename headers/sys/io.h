#ifndef SYS_IO_H
#define SYS_IO_H

// bonk enjoyer (dorito girl)
// Small header for interacting with ports

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
	asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline void outw(uint16_t port, uint16_t val) {
	asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret; 
}
static inline uint16_t inw(uint16_t port) {
	uint16_t ret;
	asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}
#endif
