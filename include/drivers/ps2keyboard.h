// bonk enjoyer (dorito girl)

// PS/2 keyboard driver

#ifndef KB_PS2
#define KB_PS2

#include <stdint.h>
#include <ports.h>
#include <layouts/kb_layouts.h>
#include <stdbool.h>
#include <drivers/tables/isr.h>

#define PS2_KB_BUFF_SIZE 512

#define LEFT_SHIFT_SC 0x2A
#define RIGHT_SHIFT_SC 0x36
#define CAPS_LOCK_SC 0x3A
#define BACKSPACE_SC 0xE
#define ENTER_SC 0x1C

// Ember2819: arrow keys for command history
#define KEY_UP    0x60
#define KEY_DOWN  0x61
// ember2819: left/right arrows for editor
#define KEY_LEFT  0x62
#define KEY_RIGHT 0x63

// Initialize PS/2 Keyboard
void ps2_kb_init();
// Process keypress
void process_keypress(scancode_t sc);
// Wait For Input
scancode_t ps2_kb_wfi();
void set_layout(KeyboardLayout layout);
//installs the handler on it s port on pic so we get interupts each time a key is pressed
void keyboard_install();
void keyboard_handler(registers_t* r);

extern char ScASCII[128];
extern char ScASCII_UPPER[128];

#endif
