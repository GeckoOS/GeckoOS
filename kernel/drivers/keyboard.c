// bonk enjoyer (dorito girl)

// PS/2 keyboard driver

#include <drivers/keyboard.h>
#include <layouts/kb_layouts.h>
#include <ports.h>
#include <terminal/terminal.h>
#include <drivers/tables/irq.h>
#include <drivers/vga.h>

// Layout map by scancodes.
// Add layout via set_layout()
static char ScASCII[128];
static char ScASCII_UPPER[128];
//a bool to listen to the input from the handler
volatile int kb_ready = 0;
//what was the last key pressED
volatile scancode_t last_scancode;
// Key State (what control keys are pressed currently)
KeyState KEYSTATE;

unsigned char scancode_to_ascii(scancode_t scancode) {
    bool shift = KEYSTATE.ShiftL || KEYSTATE.ShiftR; // Is either Left Shift or Right Shift pressed
    // If shift is pressed and CapsLock isn't, and vice versa
    if (KEYSTATE.CapsLock ^ shift)
        return ScASCII_UPPER[(uint8_t) scancode];
    else {
        return ScASCII[(uint8_t) scancode];
    }
}

void process_keypress(scancode_t sc) {
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
            if (!released) KEYSTATE.CapsLock = !KEYSTATE.CapsLock;
        default:
            break;
    }
}

scancode_t ps2_kb_wfi() {
    scancode_t scancode;
 
    
    //halts the process while kb is not ready hlt gets waken up by any interrupt including the timer
    while (!kb_ready) {
        asm volatile("hlt");

    }
    //sets ready to false
    kb_ready = 0;
    //sets the current scancode the last scancode
    scancode = last_scancode;
    
    // Ember2819: arrow key history
    if (scancode == 0xE0) {
        while (!(inb(KEYBOARD_STATUS_PORT) & 1)) {
            asm volatile("pause");
        }
        scancode_t ext = inb(KEYBOARD_DATA_PORT);
        if (ext & 0x80) return 0;        // ignore extended key releases
        if (ext == 0x48) return KEY_UP;
        if (ext == 0x50) return KEY_DOWN;
        // ember2819: left/right arrows for editor
        if (ext == 0x4B) return KEY_LEFT;
        if (ext == 0x4D) return KEY_RIGHT;
        return 0;
    }

    process_keypress(scancode);

    return scancode;
}

void ps2_kb_init() {
    KEYSTATE = (KeyState) {false, false, false, false, false, false, false};
}

// Im getting tired of writing these dumb comments that nobody reads.
// I read them...

void set_layout(KeyboardLayout layout) {
    unsigned char* lowercase = layout.lower;
    unsigned char* uppercase = layout.upper;

    for (int j = 0; j < 128; j++) ScASCII[j] = lowercase[j];
    for (int j = 0; j < 128; j++) ScASCII_UPPER[j] = uppercase[j];
}



void keyboard_handler(registers_t* r)
{
    (void)r;
    
    //listen to the key port
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    if (scancode & 0x80)
        return; // ignore key release

    last_scancode = scancode;
    kb_ready = 1;
}
//installing the handler of the pic
void keyboard_install(){
    irq_install_handler(1, keyboard_handler);
}
