#include "commands.h"
#include "colors.h"
#include "drivers/keyboard.h"
#include "drivers/vga.h"
#include "layouts/kb_layouts.h"
#include "terminal/terminal.h"
#include "comos/comos.h"
#include "mem.h"


// The command table
static Command commands[] = {
    { "help",  cmd_help  },
    { "hello", cmd_hello },
    { "contributors", cmd_contributors},
    { "setkeyswe", cmd_setkeyswe},
    { "setkeyus", cmd_setkeyus},
    { "setkeyuk", cmd_setkeyuk},
    { "clear", cmd_clear },
    { "version", cmd_version },
    { "chars", cmd_chars },
    { "comos", cmd_comos },
};

static int num_commands = sizeof(commands) / sizeof(commands[0]);

// ---- Command Functions ----

static void cmd_help(uint8_t color) {
    printf("\nhelp - Show this message\n", color);
    printf("hello - Say hello\n", color);
    printf("contributors - Display names of all contributors\n", color);
    printf("setkeyswe - Set the keyboard layout to Swedish QWERTY\n", color); // Zorx555 - Keyboard layout commands
    printf("setkeyus - Set the keyboard layout to US QWERTY\n", color);
    printf("setkeyuk - Set the keyboard layout to UK QWERTY\n", color); // MorganPG1 - Add UK Keyboard layout
    printf("clear - Clear the screen\n", color); //ember
    printf("version - Show the current version of the operating system\n", color); // TheOtterMonarch - Output version of the OS
    printf("chars - Print the available characters\n", color);
    printf("comos - Run the .comos scripting language\n", color);
    //printf("init_tables - Inits descriptor tables\n", color); // Pumpkicks - Inits the descriptor tables
    //printf("send_intr - Sends an interruption\n", color); // Pumpkicks - Sends the interruption 0x3
    //printf("start_timer - Starts a timer within 50Hz of velocity\n", color); // Pumpkicks - Starts the timer
}

static void cmd_hello(uint8_t color) {
    printf("\nHello, world!\n", color);
}

static void cmd_contributors(uint8_t color) {
    printf("\n--- Contributors ---\n", color);
    printf("Ember2819 - Founder\n", BOLD_COLOR);
    printf("Sifi11\n", color);
    printf("Crim\n", color);
    printf("CheeseFunnel23\n", color);
    printf("bonk enjoyer/dorito girl\n", BOLD_COLOR);
    printf("KaleidoscopeOld5841\n", color);
    printf("billythemoon\n", color);
    printf("TheGirl790\n", color);
    printf("kotofyt\n", color);
    printf("xtn59\n", color);
    printf("c-bass\n", color);
    printf("u/EastConsequence3792\n", color);
    printf("MorganPG1\n", color);
    printf("Zorx555\n", color);
    printf("mckaylap2304\n", color);
    printf("TheOtterMonarch\n", color);
    printf("codecrafter01001\n", color);
    printf("Pumpkicks\n", color);
}

static void cmd_setkeyswe(uint8_t color) {
    set_layout(LAYOUTS[1]); // Changed to work with my layout system
    printf("\nKeyboard layout set to Swedish QWERTY\n", color);
}

static void cmd_setkeyus(uint8_t color) {
    set_layout(LAYOUTS[0]); // Changed to work with my layout system
    printf("\nKeyboard layout set to US QWERTY\n", color);
}

static void cmd_setkeyuk(uint8_t color) { // Added by MorganPG1
    set_layout(LAYOUTS[2]); 
    printf("\nKeyboard layout set to UK QWERTY\n", color);
}

static void cmd_clear(uint8_t color) {
    terminal_clear(color);
}

static void cmd_version(uint8_t color) {
    printf("\nGeckoOS v1.0\nUsing GeckoOS Bootloader 1.0\n", color);
}

static void cmd_chars(uint8_t color) {
    printf("\n\n  ", color);
    for (int i = 1; i < 256; i++) {
        if (i == 9 || i == 10) {
            printf(" ", color);
        } else {
            char c = i;
            putchar(c, color);
        }
        printf(" ", color);
        if ((i+1)%16 == 0) {
            printf("\n", color);
        }
    }
    printf("\n", color);
}

//Ember2819,COMOS language 
static ComosState comos_state;

static void cmd_comos(uint8_t color) {
    // Demo program -- runs until FAT32 lets us load .comos files from disk
    static const char* demo =
        "print(\"\nCommunityOS scripting language (.comos)\")\n"
        "def fib(n):\n"
        "    if n <= 1:\n"
        "        return n\n"
        "    return fib(n - 1) + fib(n - 2)\n"
        "for i in range(8):\n"
        "    print(fib(i))\n";

    comos_init(&comos_state);
    comos_run(&comos_state, demo);
}

// ---- dispatcher ----
static int streq(unsigned char *a, char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == *b;
}

void run_command(unsigned char *input, uint8_t color) {
    // Check the input against command table
    for (int i = 0; i < num_commands; i++) {
        if (streq(input, commands[i].name)) {
            commands[i].func(color);
            return;
        }
    }
    if (strlen((char*)input) != 0) printf("\nUnknown command. Type 'help' for available commands\n", color);
    else printf("\n", color);
}
