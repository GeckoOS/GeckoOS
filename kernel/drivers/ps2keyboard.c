// bonk enjoyer (dorito girl)

// PS/2 keyboard driver

#include "drivers/input.h"
#include "drivers/ps2.h"
#include <drivers/ps2keyboard.h>
#include <drivers/tables/irq.h>
#include <drivers/vga.h>
#include <layouts/kb_layouts.h>
#include <ports.h>
#include <terminal/terminal.h>
// Layout map by scancodes.
// Add layout via set_layout()
// a bool to listen to the input from the handler
volatile int kb_ready = 0;
// what was the last key pressED
// Key State (what control keys are pressed currently)

void process_keypress(scancode_t sc)
{
    // Dark magic
    bool released = sc & 0x80;
    sc &= 0x7F;

    switch (sc) {
    case LEFT_SHIFT_SC:
        KEYSTATE.ShiftL = !released;
        break;
    case RIGHT_SHIFT_SC:
        KEYSTATE.ShiftR = !released;
        break;
    case CAPS_LOCK_SC:
        if (!released)
            KEYSTATE.CapsLock = !KEYSTATE.CapsLock;
    default:
        break;
    }
}

scancode_t ps2_kb_wfi()
{
    scancode_t scancode;

    // halts the process while kb is not ready hlt gets waken up by any
    // interrupt including the timer
    while (!kb_ready) {
        asm volatile("hlt");
    }
    // sets ready to false
    kb_ready = 0;
    // sets the current scancode the last scancode
    scancode = last_scancode;

    // Ember2819: arrow key history
    if (scancode == 0xE0) {
        while (!(inb(PS2_CMD_PORT) & 1)) {
            asm volatile("hlt");
        }
        scancode_t ext = inb(PS2_DATA_PORT);
        if (ext & 0x80) return 0; // ignore extended key releases
        switch (ext) {
            case 0x48: return KEY_UP;
            case 0x50: return KEY_DOWN;
            case 0x4B: return KEY_LEFT;
            case 0x4D: return KEY_RIGHT;
            default: break;
        }
        return 0;
    }

    process_keypress(scancode);

    return scancode;
}

void ps2_kb_init()
{
    KEYSTATE = (KeyState){false, false, false, false, false, false, false};
}

// Im getting tired of writing these dumb comments that nobody reads.
// I read them...

// you can make someone's day better just leaving those easter eggs :)

void keyboard_handler(registers_t *r)
{
    // listen to the key port
    uint8_t scancode = inb(PS2_DATA_PORT);

    last_scancode = scancode;
    kb_ready      = 1;

    set_layout(PS2_LAYOUTS[0]);
}
// installing the handler of the pic
void keyboard_install()
{
    irq_install_handler(1, keyboard_handler, 0);
}
