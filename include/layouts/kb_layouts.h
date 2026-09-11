#ifndef  KB_LAYOUTS_H
#define KB_LAYOUTS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    unsigned char lower[128];
    unsigned char upper[128];
    uint8_t id;
} KeyboardLayout;

extern KeyboardLayout PS2_LAYOUTS[];
extern KeyboardLayout HID_LAYOUTS[];

typedef uint8_t scancode_t;

typedef struct {
    bool ShiftL;        // Is Left Shift pressed?
    bool ShiftR;        // Is Right Shift pressed?
    bool AltL;          // Is Left Alt pressed?
    bool AltR;          // Is Right Alt pressed?
    bool CtrlL;         // Is Left Ctrl pressed?
    bool CtrlR;         // is Right Ctrl pressed?
    bool CapsLock;      // Is CapsLock pressed?
} KeyState;

extern char ScASCII[128];
extern char ScASCII_UPPER[128];
extern KeyState KEYSTATE;

void set_layout(KeyboardLayout layout);
unsigned char scancode_to_ascii(scancode_t scancode);

#endif