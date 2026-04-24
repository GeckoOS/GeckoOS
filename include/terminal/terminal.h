#ifndef TERMINAL_H
#define TERMINAL_H

#include <drivers/keyboard.h>
#include <drivers/vga.h>

#define HISTORY_SIZE 10
static unsigned char history_entries[HISTORY_SIZE][512];

#define SEVERITY_INFO 0
#define SEVERITY_DEBUG_INFO 1
#define SEVERITY_WARNING 2
#define SEVERITY_DEBUG_WARNING 3
#define SEVERITY_ERROR 4
#define SEVERITY_DEBUG_ERROR 5
#define SEVERITY_FATAL_ERROR 6

void putchar(char c, uint8_t COLOR);
void write(char* data, size_t size, uint8_t COLOR);
void printc(char* data, uint8_t COLOR);
void kprintf( int severity, char* data, ...);
void print(char* data);
void print_int(int n);
void print_hex(uint32_t n);

void vga_scroll(uint8_t color);

// Ember2819: clear command
void terminal_clear(uint8_t color);
static void history_push(unsigned char* buf);

void input(unsigned char* buff, size_t buffer_size, uint8_t color);

#endif
