#include "../terminal/terminal.h"
#include "keyboard.h"
#include "vga.h"

void PANIC(char* str) {
    print(str);
    putchar('\n', VGA_COLOR_BLACK);
    printf("PANIC", VGA_COLOR_RED);
    putchar('(', VGA_COLOR_WHITE);
    print(__FILE_NAME__);
    print(" at ");
    print_int(__LINE__);
    putchar(')', VGA_COLOR_WHITE);

    while (1) PAUSE(); // Halt for ever
}