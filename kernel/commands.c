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
    { "qoff", cmd_qoff },
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
    printf("sleep - Sleeps for 5 seconds (Finally the timer works!)\n", color); // Pumpkicks - yes
    printf("reboot - Reboots the machine\n", color); // Pumpkicks - reboots
    printf("qoff - Poweroff the QEMU emulator\n", color); // Pumpkicks - yes
    printf("ticks - Prints the timer tick\n", color); // Pumpkicks - show timer ticks
    printf("fsmount - Initialize ATA and mount the FAT16 filesystem\n", color); //Ember2819 I did all the filesystem stuff
    printf("ls - List files in the FAT16 root directory\n", color);
    printf("cat - Print a file from the FAT16 volume (prompts for name)\n", color);
    printf("fsinfo - Show FAT16 volume/BPB details\n", color);
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
static void cmd_qoff(uint8_t color) {
    // QEMU2_0_poweroff(); If you QEMU is older than 2.0, this will work
    QEMU_poweroff(); // If you QEMU is newer than 2.0, this will not work
}

static void cmd_fsmount(uint8_t color) {
    printf("\nInitializing ATA driver...\n", color);
    int found = ata_init();
    if (!found) {
        printf("ATA: No drives detected.\n", VGA_COLOR_RED);
        printf("Hint: In QEMU, add: -drive format=raw,file=fat16.img\n", color);
        return;
    }

    // Report what was found
    printf("ATA: Found ", color);
    print_int(found);
    printf(" drive(s).\n", color);
    printf("  Drive 0 (master): ", color);
    printf(ata_drive_present(ATA_DRIVE_MASTER) ? "present\n" : "not found\n", color);
    printf("  Drive 1 (slave):  ", color);
    printf(ata_drive_present(ATA_DRIVE_SLAVE)  ? "present\n" : "not found\n", color);

    if (!ata_drive_present(ATA_DRIVE_SLAVE)) {
        printf("No slave drive found. Is fat16.img attached as a second drive?\n", VGA_COLOR_RED);
        return;
    }

    printf("Mounting FAT16 on drive 1 (slave) at LBA 0...\n", color);
    if (fat16_mount(ATA_DRIVE_SLAVE, 0) != 0) {
        printf("FAT16 mount failed. Is fat16.img a valid FAT16 image?\n", VGA_COLOR_RED);
        return;
    }
    printf("FAT16 mounted successfully.\n", color);
}

static void cmd_ls(uint8_t color) {
    // (void)color; // Why?
    fat16_list_root();
}

static void cmd_cat(uint8_t color) {
    printf("\nEnter filename: ", color);

    unsigned char fname[32];
    input(fname, 32, color);
    printf("\n", color);

    FAT16_File f;
    if (fat16_open((char *)fname, &f) != 0) {
        printf("File not found: ", VGA_COLOR_RED);
        printf((char *)fname, VGA_COLOR_RED);
        printf("\n", VGA_COLOR_RED);
        return;
    }

    uint8_t readbuf[128];
    int bytes;
    while ((bytes = fat16_read(&f, readbuf, sizeof(readbuf))) > 0) {
        for (int i = 0; i < bytes; i++) {
            putchar(readbuf[i], color);
        }
    }
    printf("\n", color);
    fat16_close(&f);
}

static void cmd_fsinfo(uint8_t color) {
    const FAT16_Volume *v = fat16_get_volume();
    if (!v->mounted) {
        printf("\nFilesystem not mounted. Run 'fsmount' first.\n", VGA_COLOR_RED);
        return;
    }
    printf("\n-- FAT16 Volume Info --\n", color);
    printf("  Bytes/sector:      ", color); print_int(v->bpb.bytes_per_sector);   printf("\n", color);
    printf("  Sectors/cluster:   ", color); print_int(v->bpb.sectors_per_cluster);printf("\n", color);
    printf("  Reserved sectors:  ", color); print_int(v->bpb.reserved_sectors);   printf("\n", color);
    printf("  FATs:              ", color); print_int(v->bpb.num_fats);            printf("\n", color);
    printf("  Root entries:      ", color); print_int(v->bpb.root_entry_count);   printf("\n", color);
    printf("  Sectors/FAT:       ", color); print_int(v->bpb.sectors_per_fat);    printf("\n", color);
    printf("  Total sectors:     ", color); print_int(v->total_sectors);          printf("\n", color);
    printf("  FAT LBA:           ", color); print_int(v->fat_lba);                printf("\n", color);
    printf("  Root dir LBA:      ", color); print_int(v->root_dir_lba);           printf("\n", color);
    printf("  Data area LBA:     ", color); print_int(v->data_lba);               printf("\n", color);
    printf("-----------------------\n", color);
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
    if (strlen((char*)input) != 0) printf("\nUnknown command. Type 'help' for available commands\n", color);
    else printf("\n", color);
}
