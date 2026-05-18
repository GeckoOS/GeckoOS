#pragma once
#ifndef TERMINAL_H
#define TERMINAL_H

#include "drivers/mouse.h"
#include <drivers/keyboard.h>
#include <drivers/vga.h>

#define HISTORY_SIZE 10
static unsigned char history_entries[HISTORY_SIZE][512];

#define SEVERITY_INFO          0
#define SEVERITY_DEBUG_INFO    1
#define SEVERITY_WARNING       2
#define SEVERITY_DEBUG_WARNING 3
#define SEVERITY_ERROR         4
#define SEVERITY_DEBUG_ERROR   5
#define SEVERITY_FATAL_ERROR   6

void terminal_init();

void putchar(char c, uint8_t COLOR);
void write(char *data, size_t size, uint8_t COLOR);
void printc(char *data, uint8_t COLOR);
void kprintf(int severity, char *data, ...);
void print(char *data);
void print_int(int n);
void print_hex(uint32_t n);

void vga_scroll(uint8_t color);

// Ember2819: clear command
void terminal_clear(uint8_t color);
static void history_push(unsigned char *buf);

void input(unsigned char *buff, size_t buffer_size, uint8_t color);
void get_mouse_event(mouse_event_data_t md);

void draw_cursor(int x, int y, int visible);
// Store cursor state globally
typedef struct {
    int x;
    int y;
    int is_drawn;
    uint16_t
        original_cell; // Store the original cell at current cursor position
} cursor_t;
static float sensibility=0.7;
static cursor_t cursor = {-1, -1, 0, 0};



#endif
