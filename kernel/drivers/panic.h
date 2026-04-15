#ifndef PANIC_H
#define PANIC_H

#include "../terminal/terminal.h"
#include "tables/timer/timer.h"
#include "vga.h"
#include "../bootoptions.h"

static inline void PANIC(char* str, int line, char* file) {
    print("["); printc("PANIC", VGA_COLOR_RED); print(": at line "); print_int(line); print(" in file "); print(file); print("]: ");
    print(str); print("\nRebooting in five seconds...");

    sleep(ticks_to_seconds(5));
    reboot();
}
#define panic(msg) PANIC(msg, __LINE__, __FILE_NAME__)

#endif