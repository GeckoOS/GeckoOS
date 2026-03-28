#include "commands.h"
#include "bootoptions.h"
#include "colors.h"
#include "drivers/keyboard.h"
#include "drivers/tables/timer/timer.h"
#include "layouts/kb_layouts.h"
#include "terminal/terminal.h"
#include "comos/comos.h"
#include "mem.h"
#include "drivers/ata.h"
#include "fs/fat16.h"
#include <stdint.h>


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
    { "sleep", cmd_sleep5 },
    { "reboot", cmd_reboot },
    { "ticks", cmd_print_ticks },
    { "fsmount",   cmd_fsmount   },  // Mount the FAT16 filesystem
    { "ls",        cmd_ls        },  // List root directory
    { "cat",       cmd_cat       },  // Print a file's contents
    { "fsinfo",    cmd_fsinfo    },  // Show filesystem info (mostly for debugging)
};

static int num_commands = sizeof(commands) / sizeof(commands[0]);

// ---- Command Functions ----

static void cmd_help(uint8_t color) {
    printc("\nhelp - Show this message\n", color);
    printc("hello - Say hello\n", color);
    printc("contributors - Display names of all contributors\n", color);
    printc("setkeyswe - Set the keyboard layout to Swedish QWERTY\n", color); // Zorx555 - Keyboard layout commands
    printc("setkeyus - Set the keyboard layout to US QWERTY\n", color);
    printc("setkeyuk - Set the keyboard layout to UK QWERTY\n", color); // MorganPG1 - Add UK Keyboard layout
    printc("clear - Clear the screen\n", color); //ember
    printc("version - Show the current version of the operating system\n", color); // TheOtterMonarch - Output version of the OS
    printc("chars - Print the available characters\n", color);
    printc("comos - Run the .comos scripting language\n", color);
    printc("sleep - Sleeps for 5 seconds (Finally the timer works!)\n", color); // Pumpkicks - yes
    printc("reboot - Reboots the machine\n", color); // Pumpkicks - reboots
    printc("ticks - Prints the timer tick\n", color); // Pumpkicks - show timer ticks
    printc("fsmount - Initialize ATA and mount the FAT16 filesystem\n", color); //Ember2819 I did all the filesystem stuff
    printc("ls - List files in the FAT16 root directory\n", color);
    printc("cat - Print a file from the FAT16 volume (prompts for name)\n", color);
    printc("fsinfo - Show FAT16 volume/BPB details\n", color);
}

static void cmd_hello(uint8_t color) {
    printc("\nHello, world!\n", color);
}

static void cmd_contributors(uint8_t color) {
    printc("\n--- Contributors ---\n", color);
    printc("Ember2819 - Founder\n", BOLD_COLOR);
    printc("Sifi11\n", color);
    printc("Crim\n", color);
    printc("CheeseFunnel23\n", color);
    printc("bonk enjoyer/dorito girl\n", BOLD_COLOR);
    printc("KaleidoscopeOld5841\n", color);
    printc("billythemoon\n", color);
    printc("TheGirl790\n", color);
    printc("kotofyt\n", color);
    printc("xtn59\n", color);
    printc("c-bass\n", color);
    printc("u/EastConsequence3792\n", color);
    printc("MorganPG1\n", color);
    printc("Zorx555\n", color);
    printc("mckaylap2304\n", color);
    printc("TheOtterMonarch\n", color);
    printc("codecrafter01001\n", color);
    printc("Pumpkicks\n", color);
}

static void cmd_setkeyswe(uint8_t color) {
    set_layout(LAYOUTS[1]); // Changed to work with my layout system
    printc("\nKeyboard layout set to Swedish QWERTY\n", color);
}

static void cmd_setkeyus(uint8_t color) {
    set_layout(LAYOUTS[0]); // Changed to work with my layout system
    printc("\nKeyboard layout set to US QWERTY\n", color);
}

static void cmd_setkeyuk(uint8_t color) { // Added by MorganPG1
    set_layout(LAYOUTS[2]); 
    printc("\nKeyboard layout set to UK QWERTY\n", color);
}

static void cmd_clear(uint8_t color) {
    terminal_clear(color);
}

static void cmd_version(uint8_t color) {
    printc("\nGeckoOS v1.0\nUsing GeckoOS Bootloader 1.0\n", color);
}

static void cmd_chars(uint8_t color) {
    printc("\n\n  ", color);
    for (int i = 1; i < 256; i++) {
        if (i == 9 || i == 10) {
            printc(" ", color);
        } else {
            char c = i;
            putchar(c, color);
        }
        printc(" ", color);
        if ((i+1)%16 == 0) {
            printc("\n", color);
        }
    }
    printc("\n", color);
}
static void cmd_sleep5(uint8_t color) {
    print("\nSleeping for 5 seconds...\n");
    sleep(5);
    print("Done!\n");
}
static void cmd_reboot(uint8_t color) {
    print("\nRebooting...");
    reboot();
}
static void cmd_print_ticks(uint8_t color) {
    print("\nTick: ");
    print_int(get_tick());
    print("\n");
}

static void cmd_fsmount(uint8_t color) {
    printc("\nInitializing ATA driver...\n", color);
    int found = ata_init();
    if (!found) {
        printc("ATA: No drives detected.\n", VGA_COLOR_RED);
        printc("Hint: In QEMU, add: -drive format=raw,file=fat16.img\n", color);
        return;
    }

    // Report what was found
    printc("ATA: Found ", color);
    print_int(found);
    printc(" drive(s).\n", color);
    printc("  Drive 0 (master): ", color);
    printc(ata_drive_present(ATA_DRIVE_MASTER) ? "present\n" : "not found\n", color);
    printc("  Drive 1 (slave):  ", color);
    printc(ata_drive_present(ATA_DRIVE_SLAVE)  ? "present\n" : "not found\n", color);

    if (!ata_drive_present(ATA_DRIVE_SLAVE)) {
        printc("No slave drive found. Is fat16.img attached as a second drive?\n", VGA_COLOR_RED);
        return;
    }

    printc("Mounting FAT16 on drive 1 (slave) at LBA 0...\n", color);
    if (fat16_mount(ATA_DRIVE_SLAVE, 0) != 0) {
        printc("FAT16 mount failed. Is fat16.img a valid FAT16 image?\n", VGA_COLOR_RED);
        return;
    }
    printc("FAT16 mounted successfully.\n", color);
}

static void cmd_ls(uint8_t color) {
    (void)color;
    fat16_list_root();
}

static void cmd_cat(uint8_t color) {
    printc("\nEnter filename: ", color);

    unsigned char fname[32];
    input(fname, 32, color);
    printc("\n", color);

    FAT16_File f;
    if (fat16_open((char *)fname, &f) != 0) {
        printc("File not found: ", VGA_COLOR_RED);
        printc((char *)fname, VGA_COLOR_RED);
        printc("\n", VGA_COLOR_RED);
        return;
    }

    uint8_t readbuf[128];
    int bytes;
    while ((bytes = fat16_read(&f, readbuf, sizeof(readbuf))) > 0) {
        for (int i = 0; i < bytes; i++) {
            putchar(readbuf[i], color);
        }
    }
    printc("\n", color);
    fat16_close(&f);
}

static void cmd_fsinfo(uint8_t color) {
    const FAT16_Volume *v = fat16_get_volume();
    if (!v->mounted) {
        printc("\nFilesystem not mounted. Run 'fsmount' first.\n", VGA_COLOR_RED);
        return;
    }
    printc("\n-- FAT16 Volume Info --\n", color);
    printc("  Bytes/sector:      ", color); print_int(v->bpb.bytes_per_sector);   printc("\n", color);
    printc("  Sectors/cluster:   ", color); print_int(v->bpb.sectors_per_cluster);printc("\n", color);
    printc("  Reserved sectors:  ", color); print_int(v->bpb.reserved_sectors);   printc("\n", color);
    printc("  FATs:              ", color); print_int(v->bpb.num_fats);            printc("\n", color);
    printc("  Root entries:      ", color); print_int(v->bpb.root_entry_count);   printc("\n", color);
    printc("  Sectors/FAT:       ", color); print_int(v->bpb.sectors_per_fat);    printc("\n", color);
    printc("  Total sectors:     ", color); print_int(v->total_sectors);          printc("\n", color);
    printc("  FAT LBA:           ", color); print_int(v->fat_lba);                printc("\n", color);
    printc("  Root dir LBA:      ", color); print_int(v->root_dir_lba);           printc("\n", color);
    printc("  Data area LBA:     ", color); print_int(v->data_lba);               printc("\n", color);
    printc("-----------------------\n", color);
}

//Ember2819,COMOS language 
static ComosState comos_state;

static void cmd_comos(uint8_t color) {
    // Demo program 
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
    if (strlen((char*)input) != 0) printc("\nUnknown command. Type 'help' for available commands\n", color);
    else printc("\n", color);
}
