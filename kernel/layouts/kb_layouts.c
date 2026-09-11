// Added by MorganPG1 to make implementation of https://github.com/Ember2819/Random-People-Coding-Stuff/issues/32 easier
#include <layouts/kb_layouts.h>
#include <stdint.h>
#include <string.h>

KeyboardLayout PS2_LAYOUTS[] = {
    { // US QWERTY
        { 
            0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
            'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
            'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
            'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
            // Rest unprintable
        },
        {
            0, 27, '!', '@', '#', '$', '%','^','&','*','(',')','_','+','\b','\t',
            'Q','W','E','R','T','Y','U','I','O','P','{','}', '\n', 0,
            'A','S','D','F','G','H','J','K','L',':','"', '~', 0, '|',
            'Z','X','C','V','B','N','M','<','>','?', 0, '*', 0, ' '
            // Rest unprintable
        },
        1
    },
    { // Swedish QWERTY (added by Zorx555)
        {
            0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '+', '=', '\b', '\t',
            'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', 0x86, 0x7E, '\n', 0,
            'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 0x94, 0x84, 0x27, 0, '<',
            'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '-', 0, '*', 0, ' '
        },
        {
            0, 27, '!', '@', '#', '$', '%','&','/','(',')','=','?','+','\b','\t',
            'Q','W','E','R','T','Y','U','I','O','P',0x8F,0x7E, '\n', 0,
            'A','S','D','F','G','H','J','K','L',0x99,0x8E, '*', 0, '|',
            'Z','X','C','V','B','N','M',';',':','_', 0, '*', 0, ' '
        },
        2
    },
    { // UK QWERTY (added by MorganPG1)
        { 
            0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
            'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
            'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '#',
            'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', [86]='\\'
            // Rest unprintable
        },
        {
            0, 27, '!', '"', '$', '$', '%','^','&','*','(',')','_','+','\b','\t',
            'Q','W','E','R','T','Y','U','I','O','P','{','}', '\n', 0,
            'A','S','D','F','G','H','J','K','L',':','@', '`', 0, '~',
            'Z','X','C','V','B','N','M','<','>','?', 0, '*', 0, ' ', [86]='|'
            // Rest unprintable
        },
        3
    }
};

// HID Keyboard layout
KeyboardLayout HID_LAYOUTS[] = {
    { // English QWERTY (Pumpkicks)
        {0, 0, 0, 0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
        'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '\n', 0,
        '\b', 0, ' ', '-', '=', '[', ']', '\\', '~', ';', '\'', '`', ',', '.', '/'},
        {0, 0, 0, 0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
        'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '\n', 0,
        '\b', 0, ' ', '_', '+', '{', '}', '|', '`', ':', '"', '~', '<', '>', '?'},
        4
    }
};

char ScASCII[128];
char ScASCII_UPPER[128];

KeyState KEYSTATE;
uint8_t ActualLayoutId = 0;

unsigned char scancode_to_ascii(scancode_t scancode)
{
    if (scancode > 128 || scancode < 0) return 0;

    bool shift = KEYSTATE.ShiftL ||
                 KEYSTATE.ShiftR; // Is either Left Shift or Right Shift pressed
    // If shift is pressed and CapsLock isn't, and vice versa

    if (KEYSTATE.CapsLock ^ shift)
        return ScASCII_UPPER[scancode];
    else {
        return ScASCII[scancode];
    }
}

void set_layout(KeyboardLayout layout)
{
    if (ActualLayoutId == layout.id) return;

    memcpy(ScASCII, layout.lower, sizeof(ScASCII));
    memcpy(ScASCII_UPPER, layout.upper, sizeof(ScASCII_UPPER));
    ActualLayoutId = layout.id;
}