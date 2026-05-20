#pragma once

#include <stdint.h>
#include <ports.h>

#define COM1 0x3F8

static int serial_initialized = 0;

static int serial_can_send(void) {
    return (inb(COM1 + 5) & 0x20) != 0;
}

static void serial_putc(char c);
static void serial_puts(const char *str);

static void serial_init(void) {
    if (serial_initialized) return;

    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
    serial_initialized = 1;
}

static void serial_putc(char c) {
    if (!serial_initialized) serial_init();
    if (!serial_initialized) return;
    while (!serial_can_send());
    outb(COM1 + 0, c);
}

static void serial_puts(const char *str) {
    while (*str) serial_putc(*str++);
}

static void serial_put_hex(uint64_t val) {
    const char *hex = "0123456789abcdef";
    serial_puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        serial_putc(hex[(val >> i) & 0xF]);
    }
}

static void serial_put_dec(uint32_t val) {
    char buf[16];
    int i = 0;
    if (val == 0) {
        serial_putc('0');
        return;
    }
    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    while (i > 0) serial_putc(buf[--i]);
}
