#pragma once
#include <stdint.h>
#ifndef TERMINAL_H
#define TERMINAL_H

#include "drivers/mouse.h"
#include <drivers/keyboard.h>
#include <drivers/vga.h>

#define VGA_MEMORY ((uint16_t *)0xB8000)

/* vga.h defines VGA_TEXT_WIDTH/HEIGHT for the legacy 80x25 text mode.
 * We redefine them here for the framebuffer terminal (1024x768 / 8x16 font).
 * Undef first to suppress -Wmacro-redefined. */
#undef  VGA_TEXT_WIDTH
#define VGA_TEXT_WIDTH  128   /* 1024px / 8px font  = 128 cols */
#undef  VGA_TEXT_HEIGHT
#define VGA_TEXT_HEIGHT 48    /* 768px  / 16px font = 48 rows  */

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

;

#define SCROLLBACK_LINES 1000 /* 1000 * 128 cols * 2 bytes = 256KB in BSS */
static uint16_t console_history[SCROLLBACK_LINES][VGA_TEXT_WIDTH]; //console history
static int total_lines=0;//total lines in history should be update when terminal row is
static int viewport_top=0;//viewport lines(the current line we at should be )
//renders to vga
void render_viewport();
//scrolls up
void scroll_up();
//scrolls down
void scroll_down();

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
static float sensibility = 0.7;
static cursor_t cursor   = {-1, -1, 0, 0};


void add_to_history(void);

#endif