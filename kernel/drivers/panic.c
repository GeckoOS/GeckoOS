#include "../terminal/terminal.h"
#include "keyboard.h"
#include "vga.h"

void PANIC(char* str, int line) {
    print(str);
    putchar('\n', VGA_COLOR_BLACK);
    printc("PANIC", VGA_COLOR_RED);
    putchar('(', VGA_COLOR_WHITE);
    print(__FILE_NAME__);
    print(" at line ");
    print_int(line);
    putchar(')', VGA_COLOR_WHITE);

    while (1) PAUSE(); // Halt forever
}