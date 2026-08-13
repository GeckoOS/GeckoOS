#include "exe.h"
#include "process/process.h"
#include "terminal/printf.h"
#include <commands.h>
#include <bootoptions.h>
#include <colors.h>
#include <drivers/keyboard.h>
#include <drivers/tables/timer.h>
#include <drivers/serial.h>
#include <layouts/kb_layouts.h>
#include <terminal/terminal.h>
#include <gk/gk.h>
#include <mem.h>
#include <drivers/ata.h>
#include <fs/fs.h>
#include <fs/fat32.h>
#include <stdint.h>
#include <drivers/pci.h>
#include <stdbool.h>
#include <net/net.h>
#include <net/icmp.h>
#include <net/arp.h>
#include <net/udp.h>
#include <net/dns.h>
#include <drivers/e1000.h>

// Forward decls
static void cmd_udpsend(char *args, uint8_t color);
static void cmd_dns(char *args, uint8_t color);
static void cmd_listhostfiles(char *args, uint8_t color);
static void cmd_hostfilesize(char *args, uint8_t color);
static void cmd_pullhostfile(char *args, uint8_t color);
static const char* starts_with(const unsigned char* str, const char* prefix);


// Helper to get Nth argument from args string
static void get_arg(const char* args, int arg_num, char* dest, int max_len) {
    if (!args) { dest[0] = '\0'; return; }
    int current_arg = 0;
    const char* p = args;
    while (*p) {
        while (*p == ' ') p++; // skip leading spaces
        if (*p == '\0') break;
        
        if (current_arg == arg_num) {
            int len = 0;
            while (*p != ' ' && *p != '\0' && len < max_len - 1) {
                dest[len++] = *p++;
            }
            dest[len] = '\0';
            return;
        }
        
        while (*p != ' ' && *p != '\0') p++;
        current_arg++;
    }
    dest[0] = '\0';
}

// Command table
static Command commands[] = {
    // --- system / info ---
    { "help",         cmd_help         },
    { "hello",        cmd_hello        },
    { "contributors", cmd_contributors },
    { "clear",        cmd_clear        },
    { "version",      cmd_version      },
    { "chars",        cmd_chars        },
    { "uptime",       cmd_uptime       },
    { "meminfo",      cmd_meminfo      },
    { "lspci",        cmd_lspci        },
    { "processes",    cmd_processes    },
    // --- keyboard ---
    { "setkeyswe",    cmd_setkeyswe     },
    { "setkeyus",     cmd_setkeyus     },
    { "setkeyuk",     cmd_setkeyuk     },
    // --- timer / power ---
    { "sleep",        cmd_sleep5       },
    { "reboot",       cmd_reboot       },
    { "ticks",        cmd_print_ticks  },
    // --- scripting ---
    { "gk",           cmd_gk           },
    // --- filesystem ---
    { "fsmount",      cmd_fsmount      },
    { "ls",           cmd_ls           },
    { "cat",          cmd_cat          },
    { "fsinfo",       cmd_fsinfo       },
    { "touch",        cmd_touch        },
    { "rm",           cmd_rm           },
    { "cp",           cmd_cp           },
    { "mv",           cmd_mv           },
    { "mkdir",        cmd_mkdir        },
    { "echo",         cmd_echo         },
    { "write",        cmd_write        },
    { "dumpelf",      cmd_dumpelf      },
    { "runelf",       cmd_runelf       },
    // --- network ---
    { "ping",         cmd_ping            },
    { "udpsend",      cmd_udpsend         },
    { "dns",          cmd_dns             },
    { "listhostfiles", cmd_listhostfiles  },
    { "hostfilesize",  cmd_hostfilesize   },
    { "pullhostfile",  cmd_pullhostfile   }
};

static int num_commands = sizeof(commands) / sizeof(commands[0]);

struct drive_fs_t *fs;

static const char* help_lines[] = {
    "--- System ---",
    "help         - Show this message",
    "hello        - Say hello",
    "contributors - List contributors",
    "clear        - Clear the screen",
    "version      - OS version",
    "chars        - Print available characters",
    "uptime       - Show system uptime (ticks)",
    "meminfo      - Show memory info",
    "lspci        - List PCI devices",
    "processes    - Show the count of active processes",
    "",
    "--- Keyboard ---",
    "setkeyswe - Swedish QWERTY layout",
    "setkeyus  - US QWERTY layout",
    "setkeyuk  - UK QWERTY layout",
    "",
    "--- Timer / Power ---",
    "sleep  - Sleep 5 seconds",
    "ticks  - Print timer tick count",
    "reboot - Reboot",
    "",
    "--- Scripting ---",
    "gk        - GK scripting language (demo)",
    "gk <file> - Run a .gk script from the FS",
    "",
    "--- FAT32 Filesystem ---",
    "fsmount                - Mount the FAT32 data disk (fat32.img)",
    "ls                     - List files in root directory",
    "cat <file>             - Read and display a file",
    "fsinfo                 - Show volume info (label, size, clusters)",
    "touch <file> <content> - Create a new file with content",
    "rm <file>              - Delete a file",
    "cp <src> <dst>         - Copy a file to a new name",
    "mv <src> <dst>         - Move/rename a file",
    "mkdir <dir>            - Create a new directory",
    "echo <text>            - Print text to screen",
    "write <file> <text>    - Append text to an existing file",
    "dumpelf <file>         - Dumps an ELF file",
    "runelf <file>          - Runs an ELF file",
    "",
    "--- Network ---",
    "ping <ip>                   - Ping an IP address (e.g. ping 10.0.2.2)",
    "udpsend <ip> <payload>      - Sends a UDP message to a specified IP",
    "dns <hostname>              - Uses the DNS 8.8.4.4 to get the IP of a host",
    "listhostfiles               - List files available on the host",
    "hostfilesize <name>         - Show the size of a file on the host",
    "pullhostfile <name> [local] - Download a file from the host (10.0.2.2:8080)"
    "",
    0
};

#define SPACE_SC 0x39

static void cmd_help(char *args, uint8_t color) {
    (void)args;
    int num_lines = 0;
    while (help_lines[num_lines] != 0) num_lines++;

    int rows = terminal_rows() - 2;   /* leave room for the footer */
    if (rows > 40) rows = 40;

    int sheet_start[16];
    int sheet_len[16];
    int n_sheets = 0;
    int cur = 0;
    for (int i = 0; i < num_lines; i++) {
        if (help_lines[i][0] == '\0') {
            if (i > cur) {
                sheet_start[n_sheets] = cur;
                sheet_len[n_sheets] = i - cur;
                n_sheets++;
            }
            cur = i + 1;
        }
    }
    if (cur < num_lines) {
        sheet_start[n_sheets] = cur;
        sheet_len[n_sheets] = num_lines - cur;
        n_sheets++;
    }

    int page_start[16];
    int page_end[16];
    int pages = 0;
    int lines = 0;
    int first = -1;
    for (int s = 0; s < n_sheets; s++) {
        if (first < 0) { first = s; lines = 0; }
        if (first != s && lines + sheet_len[s] > rows) {
            page_start[pages] = sheet_start[first];
            page_end[pages]   = sheet_start[s];
            pages++;
            first = s;
            lines = 0;
        }
        lines += sheet_len[s];
    }
    if (first >= 0) {
        page_start[pages] = sheet_start[first];
        page_end[pages]   = num_lines;
        pages++;
    }

    int page = 0;

    while (1) {
        terminal_clear(color);
        for (int k = page_start[page]; k < page_end[page]; k++) {
            if (help_lines[k][0] == '\0') {
                printc("\n", color);
                continue;
            }
            if (help_lines[k][0] == '-') {
                printc((char*)help_lines[k], VGA_COLOR_LIGHT_CYAN);
            } else {
                printc((char*)help_lines[k], color);
            }
            printc("\n", color);
        }

        printc("\n", color);
        printc("[Space: next page  Enter: exit]  Page ", VGA_COLOR_DARK_GREY);
        print_int(page + 1);
        printc("/", VGA_COLOR_DARK_GREY);
        print_int(pages);
        printc("\n", color);

        while (1) {
            scancode_t sc = ps2_kb_wfi();
            if (sc & 0x80) continue;
            if (sc == 0) continue;
            if (sc == ENTER_SC) return;
            if (sc == SPACE_SC) {
                page = (page + 1) % pages;
                break;
            }
        }
    }
}

static void cmd_hello(char *args, uint8_t color) {
    (void)args;
    printc("Hello, world!\n", color);
}

static void cmd_contributors(char *args, uint8_t color) {
    (void)args;
    printc("--- Contributors ---\n", color);
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
    printc("DarkThemeGeek\n", color);
    printc("nfoxers\n", color);
    printc("slothscript\n", color);
}

static void cmd_setkeyswe(char *args, uint8_t color) {
    (void)args;
    set_layout(LAYOUTS[1]);
    printc("Keyboard layout set to Swedish QWERTY\n", color);
}

static void cmd_setkeyus(char *args, uint8_t color) {
    (void)args;
    set_layout(LAYOUTS[0]);
    printc("Keyboard layout set to US QWERTY\n", color);
}

static void cmd_setkeyuk(char *args, uint8_t color) {
    (void)args;
    set_layout(LAYOUTS[2]);
    printc("Keyboard layout set to UK QWERTY\n", color);
}

static void cmd_clear(char *args, uint8_t color) {
    (void)args;
    terminal_clear(color);
}

static void cmd_version(char *args, uint8_t color) {
    (void)args;
    printc("GeckoOS v2.0 (GRUB/Multiboot2)\n", color);
}

static void cmd_chars(char *args, uint8_t color) {
    (void)args;
    for (int i = 1; i < 256; i++) {
        if (i == 9 || i == 10) {
            printc(" ", color);
        } else {
            char c = i;
            putchar(c, color);
        }
        printc(" ", color);
        if ((i+1)%16 == 0) printc("\n", color);
    }
    printc("\n", color);
}

static void cmd_sleep5(char *args, uint8_t color) {
    (void)args;
    printc("Sleeping for 5 seconds...\n", color);
    sleep(5);
    printc("Done!\n", color);
}

static void cmd_reboot(char *args, uint8_t color) {
    (void)args;
    printc("Rebooting...", color);
    reboot();
}

static void cmd_print_ticks(char *args, uint8_t color) {
    (void)args;
    printc("Tick: ", color);
    print_int(get_tick());
    printc("\n", color);
}

static void cmd_fsmount(char *args, uint8_t color) {
    (void)args;
    if (!get_kdrive(1)) {
        printc("No slave drive found. Is fat32.img attached as a second drive?\n", VGA_COLOR_RED);
        return;
    }
    fs = fs_drive_open(get_kdrive(1));
    if (fs == 0) {
        printc("Filesystem mount failed. Is fat32.img a valid FAT32 image?\n", VGA_COLOR_RED);
        return;
    }
    printc("Filesystem mounted successfully.\n", color);
}

static void cmd_ls(char *args, uint8_t color) {
    struct fs_entries_t entries;
    int i;
    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }
    entries = fs->get_entries((void*)fs);
    for (i = 0; i < (int)entries.count; i++) {
        switch(entries.entries[i].type) {
        case ENTRY_FILE:      printc("[FILE] ", VGA_COLOR_LIGHT_GREEN); break;
        case ENTRY_DIRECTORY: printc("[DIR]  ", VGA_COLOR_LIGHT_BLUE);  break;
        default: break;
        }
        printc(entries.entries[i].dir.name, color);
        printc("\n", color);
    }
}

static void cmd_cat(char *args, uint8_t color) {
    struct fs_entries_t entries;
    unsigned char fname[32];
    int i, found;

    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }

    get_arg(args, 0, (char*)fname, 32);
    if (fname[0] == '\0') {
        printc("Usage: cat <filename>\n", VGA_COLOR_RED);
        return;
    }

    entries = fs->get_entries((void*)fs);
    found = -1;
    for (i = 0; i < (int)entries.count; i++) {
        if (entries.entries[i].type != ENTRY_FILE) continue;
        const char *a = entries.entries[i].file.name;
        const unsigned char *b = fname;
        int match = 1;
        while (*a && *b) {
            char ca = (*a >= 'a' && *a <= 'z') ? *a - 32 : *a;
            char cb = (*b >= 'a' && *b <= 'z') ? *b - 32 : *b;
            if (ca != cb) { match = 0; break; }
            a++; b++;
        }
        if (match && *a == '\0' && *b == '\0') { found = i; break; }
    }

    if (found < 0) {
        printc("File not found: ", VGA_COLOR_RED);
        printc((char*)fname, VGA_COLOR_RED);
        printc("\n", color);
        return;
    }

    uint8_t readbuf[128];
    int bytes, j = 0;
    while ((bytes = entries.entries[found].file.read(
            (void*)&entries.entries[found].file, j * 128, 128, readbuf)) > 0) {
        j++;
        for (int k = 0; k < bytes; k++)
            putchar(readbuf[k], color);
    }
    printc("\n", color);
}

static void cmd_fsinfo(char *args, uint8_t color) {
    (void)args;
    if (!fs) {
        printc("Filesystem not mounted. Run 'fsmount' first.\n", VGA_COLOR_RED);
        return;
    }
    printc("\n", color);
    fat32_print_info(fs, color);
}

static void cmd_touch(char *args, uint8_t color) {
    unsigned char fname[32];
    unsigned char content[256];

    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }

    get_arg(args, 0, (char*)fname, 32);
    get_arg(args, 1, (char*)content, 255);

    if (fname[0] == '\0') {
        printc("Usage: touch <filename> <content>\n", VGA_COLOR_RED);
        return;
    }

    int result = fat32_create_file(fs, (char*)fname,
                                   (const uint8_t*)content, strlen((char*)content));
    if (result == 0) {
        printc("File created: ", color);
        printc((char*)fname, color);
        printc("\n", color);
    } else {
        printc("Failed to create file (disk full or root dir full?)\n", VGA_COLOR_RED);
    }
}

static void cmd_rm(char *args, uint8_t color) {
    unsigned char fname[32];
    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }

    get_arg(args, 0, (char*)fname, 32);
    if (fname[0] == '\0') {
        printc("Usage: rm <filename>\n", VGA_COLOR_RED);
        return;
    }

    printc("Delete ", color);
    printc((char*)fname, color);
    printc("? (y/n): ", VGA_COLOR_LIGHT_RED);
    
    unsigned char confirm[4];
    input(confirm, 4, color);
    printc("\n", color);
    if (confirm[0] != 'y' && confirm[0] != 'Y') {
        printc("Cancelled.\n", color);
        return;
    }

    int result = fat32_delete_file(fs, (char*)fname);
    if (result == 0) {
        printc("Deleted: ", color);
        printc((char*)fname, color);
        printc("\n", color);
    } else {
        printc("Failed to delete (file not found or FS error).\n", VGA_COLOR_RED);
    }
}

static void cmd_cp(char *args, uint8_t color) {
    unsigned char src[32], dst[32];
    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }

    get_arg(args, 0, (char*)src, 32);
    get_arg(args, 1, (char*)dst, 32);

    if (src[0] == '\0' || dst[0] == '\0') {
        printc("Usage: cp <src> <dst>\n", VGA_COLOR_RED);
        return;
    }

    struct fs_entries_t entries = fs->get_entries((void*)fs);
    int found = -1;
    for (int i = 0; i < (int)entries.count; i++) {
        if (entries.entries[i].type != ENTRY_FILE) continue;
        const char *a = entries.entries[i].file.name;
        const unsigned char *b = src;
        int match = 1;
        while (*a && *b) {
            char ca = (*a >= 'a' && *a <= 'z') ? *a - 32 : *a;
            char cb = (*b >= 'a' && *b <= 'z') ? *b - 32 : *b;
            if (ca != cb) { match = 0; break; }
            a++; b++;
        }
        if (match && *a == '\0' && *b == '\0') { found = i; break; }
    }

    if (found < 0) {
        printc("Source not found.\n", VGA_COLOR_RED);
        return;
    }

    static uint8_t copybuf[4096];
    int total = 0, chunk, j = 0;
    while (total < 4096) {
        uint8_t tmp[128];
        chunk = entries.entries[found].file.read(
            (void*)&entries.entries[found].file, j * 128, 128, tmp);
        if (chunk <= 0) break;
        for (int k = 0; k < chunk && total < 4096; k++)
            copybuf[total++] = tmp[k];
        j++;
    }

    int result = fat32_create_file(fs, (char*)dst, copybuf, total);
    if (result == 0) {
        printc("Copied to: ", color);
        printc((char*)dst, color);
        printc("\n", color);
    } else {
        printc("Copy failed.\n", VGA_COLOR_RED);
    }
}

static void cmd_mv(char *args, uint8_t color) {
    unsigned char src[32], dst[32];
    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }

    get_arg(args, 0, (char*)src, 32);
    get_arg(args, 1, (char*)dst, 32);

    if (src[0] == '\0' || dst[0] == '\0') {
        printc("Usage: mv <src> <dst>\n", VGA_COLOR_RED);
        return;
    }

    struct fs_entries_t entries = fs->get_entries((void*)fs);
    int found = -1;
    for (int i = 0; i < (int)entries.count; i++) {
        if (entries.entries[i].type != ENTRY_FILE) continue;
        const char *a = entries.entries[i].file.name;
        const unsigned char *b = src;
        int match = 1;
        while (*a && *b) {
            char ca = (*a >= 'a' && *a <= 'z') ? *a - 32 : *a;
            char cb = (*b >= 'a' && *b <= 'z') ? *b - 32 : *b;
            if (ca != cb) { match = 0; break; }
            a++; b++;
        }
        if (match && *a == '\0' && *b == '\0') { found = i; break; }
    }

    if (found < 0) { printc("Source not found.\n", VGA_COLOR_RED); return; }

    static uint8_t mvbuf[4096];
    int total = 0, chunk, j = 0;
    while (total < 4096) {
        uint8_t tmp[128];
        chunk = entries.entries[found].file.read(
            (void*)&entries.entries[found].file, j * 128, 128, tmp);
        if (chunk <= 0) break;
        for (int k = 0; k < chunk && total < 4096; k++)
            mvbuf[total++] = tmp[k];
        j++;
    }

    int rc = fat32_create_file(fs, (char*)dst, mvbuf, total);
    if (rc != 0) { printc("Move failed (could not create dst).\n", VGA_COLOR_RED); return; }

    fat32_delete_file(fs, (char*)src);
    printc("Moved: ", color);
    printc((char*)src, color);
    printc(" -> ", color);
    printc((char*)dst, color);
    printc("\n", color);
}

static void cmd_mkdir(char *args, uint8_t color) {
    unsigned char dname[32];
    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }

    get_arg(args, 0, (char*)dname, 32);
    if (dname[0] == '\0') {
        printc("Usage: mkdir <directory_name>\n", VGA_COLOR_RED);
        return;
    }

    int result = fat32_mkdir(fs, (char*)dname);
    if (result == 0) {
        printc("Directory created: ", color);
        printc((char*)dname, color);
        printc("\n", color);
    } else {
        printc("Failed to create directory.\n", VGA_COLOR_RED);
    }
}

static void cmd_echo(char *args, uint8_t color) {
    printc(args, color);
    printc("\n", color);
}

static void cmd_write(char *args, uint8_t color) {
    unsigned char fname[32];
    unsigned char content[256];
    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }

    get_arg(args, 0, (char*)fname, 32);
    get_arg(args, 1, (char*)content, 255);

    if (fname[0] == '\0') {
        printc("Usage: write <filename> <text>\n", VGA_COLOR_RED);
        return;
    }

    int result = fat32_append_file(fs, (char*)fname,
                                   (const uint8_t*)content, strlen((char*)content));
    if (result == 0) {
        printc("Appended to: ", color);
        printc((char*)fname, color);
        printc("\n", color);
    } else {
        printc("Failed (file not found or FS error).\n", VGA_COLOR_RED);
    }
}

static void cmd_uptime(char *args, uint8_t color) {
    (void)args;
    uint32_t ticks = get_tick();
    uint32_t seconds = ticks / 50;
    uint32_t minutes = seconds / 60;
    uint32_t hours   = minutes / 60;
    seconds %= 60;
    minutes %= 60;

    printc("Uptime: ", color);
    print_int(hours);
    printc("h ", color);
    print_int(minutes);
    printc("m ", color);
    print_int(seconds);
    printc("s  (", color);
    print_int(ticks);
    printc(" ticks)\n", color);
}

static void cmd_meminfo(char *args, uint8_t color) {
    (void)args;
    printc("Memory:\n", color);
    printc("  Heap base : 0x200000\n", color);
    printc("  Heap end  : 0x500000 (3 MB window, hardcoded)\n", color);
    printc("  TODO: wire up Multiboot2 memory map (Phase 1)\n", color);
}

static GkState gk_state;

static void cmd_gk(char *args, uint8_t color) {
    if (*args == '\0') {
        printc("GeckoOS scripting language is running!\n", color);
    } else {
        cmd_gk_run_file(args, color);
    }
}

static void cmd_gk_run_file(const char* filename, uint8_t color) {
    if (!fs) {
        printc("Filesystem not mounted. Run 'fsmount' first.\n", VGA_COLOR_RED);
        return;
    }

    struct fs_entries_t entries = fs->get_entries((void*)fs);

    int found = -1;
    for (int i = 0; i < (int)entries.count; i++) {
        if (entries.entries[i].type != ENTRY_FILE) continue;
        const char *a = entries.entries[i].file.name;
        const char *b = filename;
        int match = 1;
        while (*a && *b) {
            char ca = (*a >= 'a' && *a <= 'z') ? *a - 32 : *a;
            char cb = (*b >= 'a' && *b <= 'z') ? *b - 32 : *b;
            if (ca != cb) { match = 0; break; }
            a++; b++;
        }
        if (match && *a == '\0' && *b == '\0') { found = i; break; }
    }

    if (found < 0) {
        printc("File not found: ", VGA_COLOR_RED);
        printc((char*)filename, VGA_COLOR_RED);
        printc("\n", color);
        return;
    }

    static char src_buf[GK_MAX_SRC];
    int total = 0, chunk, offset = 0;
    while (total < GK_MAX_SRC - 1) {
        uint8_t tmp[128];
        chunk = entries.entries[found].file.read(
            (void*)&entries.entries[found].file, offset, 128, tmp);
        if (chunk <= 0) break;
        for (int k = 0; k < chunk && total < GK_MAX_SRC - 1; k++)
            src_buf[total++] = (char)tmp[k];
        offset += chunk;
    }
    src_buf[total] = '\0';

    gk_init(&gk_state);
    gk_run(&gk_state, src_buf);
}

static void cmd_lspci(char *args, uint8_t color) {
    (void)args;
    pci_lspci();
}

static uint32_t parse_ip(const char *str) {
    uint32_t a = 0, b = 0, c = 0, d = 0;
    int i = 0;
    while (*str >= '0' && *str <= '9') { a = a * 10 + (*str - '0'); str++; }
    if (*str == '.') str++; i++;
    while (*str >= '0' && *str <= '9') { b = b * 10 + (*str - '0'); str++; }
    if (*str == '.') str++; i++;
    while (*str >= '0' && *str <= '9') { c = c * 10 + (*str - '0'); str++; }
    if (*str == '.') str++; i++;
    while (*str >= '0' && *str <= '9') { d = d * 10 + (*str - '0'); str++; }
    (void)i;
    return IP(a, b, c, d);
}

static void print_ip(uint32_t ip) {
    print_int((ip >> 24) & 0xFF);
    printc(".", VGA_COLOR_LIGHT_GREY);
    print_int((ip >> 16) & 0xFF);
    printc(".", VGA_COLOR_LIGHT_GREY);
    print_int((ip >> 8) & 0xFF);
    printc(".", VGA_COLOR_LIGHT_GREY);
    print_int(ip & 0xFF);
}

static void process_rx_packets(void) {
    uint8_t buf[2048];
    uint16_t len;
    while (e1000_receive(buf, &len) == 0) {
        net_handle_packet(buf, len);
    }
}

static void cmd_ping(char *args, uint8_t color) {
    unsigned char ip_str[32];
    get_arg(args, 0, (char*)ip_str, 32);

    if (ip_str[0] == '\0') {
        printc("Usage: ping <ip> (e.g. ping 10.0.2.2)\n", VGA_COLOR_LIGHT_GREY);
        printc("Our IP: ", VGA_COLOR_LIGHT_CYAN);
        print_ip(net_ip);
        printc("\n", VGA_COLOR_LIGHT_GREY);
        return;
    }

    uint32_t target_ip = parse_ip((char *)ip_str);
    printc("Pinging ", color);
    print_ip(target_ip);
    printc(" ...\n", color);

    // Off-subnet packets are sent to the default gateway at layer 2
    uint32_t next_hop = ((target_ip & net_subnet) == (net_ip & net_subnet))
        ? target_ip
        : net_gateway;

    arp_request(next_hop);

    int tick_start = get_tick();
    int resolved = 0;
    uint8_t mac[6];

    while (get_tick() - tick_start < 100) {
        process_rx_packets();
        if (arp_resolve(next_hop, mac)) {
            resolved = 1;
            break;
        }
    }

    if (!resolved) {
        printc("ARP timeout - could not resolve MAC address\n", VGA_COLOR_RED);
        return;
    }

    printc("ARP resolved, sending echo requests...\n", VGA_COLOR_LIGHT_GREEN);

    int sent = 0;
    int received = 0;
    uint16_t ping_id = 0x1234;

    for (int i = 0; i < 4; i++) {
        icmp_clear_reply();
        icmp_send_echo_request(target_ip, ping_id, i);
        sent++;

        int wait_start = get_tick();
        int got_reply = 0;

        while (get_tick() - wait_start < 100) {
            process_rx_packets();
            if (icmp_got_reply()) {
                got_reply = 1;
                break;
            }
        }

        if (got_reply) {
            received++;
            serial_puts("Reply from ");
            serial_put_dec((target_ip >> 24) & 0xFF);
            serial_putc('.');
            serial_put_dec((target_ip >> 16) & 0xFF);
            serial_putc('.');
            serial_put_dec((target_ip >> 8) & 0xFF);
            serial_putc('.');
            serial_put_dec(target_ip & 0xFF);
            serial_puts("\n");
            printc("Reply from ", VGA_COLOR_LIGHT_GREEN);
            print_ip(target_ip);
            printc("\n", color);
        } else {
            serial_puts("Request timed out\n");
            printc("Request timed out\n", VGA_COLOR_RED);
        }

        timer_wait(50);
    }

    serial_puts("\n--- Ping Statistics ---\n");
    serial_puts("Sent: ");
    serial_put_dec(sent);
    serial_puts(", Received: ");
    serial_put_dec(received);
    serial_puts(", Lost: ");
    serial_put_dec(sent - received);
    serial_puts("\n");

    printc("\n--- Ping Statistics ---\n", VGA_COLOR_LIGHT_CYAN);
    printc("Sent: ", color);
    print_int(sent);
    printc(", Received: ", color);
    print_int(received);
    printc(", Lost: ", color);
    print_int(sent - received);
    printc("\n", color);
}

static void cmd_udpsend(char *args, uint8_t color) {
    // args: <IP> <payload>
    unsigned char ip_str[64];
    unsigned char payload[512];

    get_arg(args, 0, (char*)ip_str, 32);
    get_arg(args, 1, (char*)payload, 512);

    if (ip_str[0] == '\0' || payload[0] == '\0') {
        printc("Usage: udpsend <ip> <payload>\n", VGA_COLOR_LIGHT_GREY);
        return;
    }

    // support ip:port as first arg
    uint16_t dst_port = 53; // default destination port
    uint32_t target_ip;

    // look for optional :port suffix (local helpers; freestanding libc has no
    // strchr/atoi).
    char *colon = 0;
    for (char *q = (char*)ip_str; *q; q++) {
        if (*q == ':') { colon = q; break; }
    }
    if (colon) {
        *colon = '\0';
        int p = 0;
        for (char *q = colon + 1; *q >= '0' && *q <= '9'; q++) {
            p = p * 10 + (*q - '0');
        }
        if (p > 0 && p <= 65535) dst_port = (uint16_t)p;
    }

    target_ip = parse_ip((char*)ip_str);
    // use ephemeral source port by default
    uint16_t src_port = 49152;

    printc("Sending UDP to ", color);
    print_ip(target_ip);
    printc(":", color);
    print_int(dst_port);
    printc("\n", color);

    // Send the datagram and start listening for a reply. We pump inbound
    // packets and poll for the reply ourselves.
    if (udp_request_start(target_ip, src_port, dst_port,
                          (char*)payload, strlen((char*)payload)) != 0) {
        printc("Failed to send UDP\n", VGA_COLOR_RED);
        return;
    }

    udp_reply_t reply;
    int got_reply = 0;
    int start = get_tick();
    while (get_tick() - start < 100) {
        process_rx_packets();
        if (udp_request_try_reply(&reply) == 0) {
            got_reply = 1;
            break;
        }
        timer_wait(1);
    }
    udp_request_finish();

    if (got_reply) {
        printc("Reply from ", VGA_COLOR_LIGHT_GREEN);
        print_ip(reply.src_ip);
        printc(":", color);
        print_int(reply.src_port);
        printc(" (", color);
        print_int(reply.payload_len);
        printc(" bytes): ", color);
        for (uint16_t k = 0; k < reply.payload_len; k++) {
            char c = (char)reply.payload[k];
            putchar(c, color);
        }
        printc("\n", color);
    } else {
        printc("No reply received (timeout)\n", VGA_COLOR_LIGHT_GREY);
    }
}

// DNS lookup via 8.8.4.4. Sends a UDP query to port 53 and waits for a
// reply on an ephemeral port, then prints the first A record answer.
static void cmd_dns(char *args, uint8_t color) {
    (void)args;
    (void)color;

    char *name = args;

    if (name[0] == '\0') {
        printf("Usage: dns <hostname>\n");
        return;
    }

    send_dns(name);

    udp_reply_t reply;
    int got_reply = 0;
    int start = get_tick();
    while (get_tick() - start < 100) {
        process_rx_packets();
        if (udp_request_try_reply(&reply) == 0) {
            got_reply = 1;
            break;
        }
        timer_wait(1);
    }
    udp_request_finish();

    if (got_reply) {
        uint32_t ip;
        if (dns_parse_reply(reply.payload, reply.payload_len, &ip) == 0) {
            printc(name, color);
            printc(" = ", VGA_COLOR_LIGHT_GREY);
            print_ip(ip);
            printc("\n", color);
            return;
        }
        int rcode = dns_reply_rcode(reply.payload, reply.payload_len);
        if (rcode > 0) {
            const char *why = "error";
            switch (rcode) {
            case 1: why = "FORMERR (malformed query)";  break;
            case 2: why = "SERVFAIL (server failed)";   break;
            case 3: why = "NXDOMAIN (no such domain)";  break;
            case 4: why = "NOTIMP (not implemented)";   break;
            case 5: why = "REFUSED";                    break;
            }
            printc(name, color);
            printc(": ", VGA_COLOR_LIGHT_GREY);
            printc(why, VGA_COLOR_RED);
            printc("\n", color);
            return;
        }
        static const char dns_hex[] = "0123456789abcdef";
        printc("Reply from ", VGA_COLOR_LIGHT_GREEN);
        print_ip(reply.src_ip);
        printc(":", color);
        print_int(reply.src_port);
        printc(" (", color);
        print_int(reply.payload_len);
        printc(" bytes):\n", color);
        for (uint16_t k = 0; k < reply.payload_len; k++) {
            putchar(dns_hex[(reply.payload[k] >> 4) & 0xF], color);
            putchar(dns_hex[reply.payload[k] & 0xF], color);
            putchar((k % 16 == 15) ? '\n' : ' ', color);
        }
        printc("\n", color);
    } else {
        printc("No reply received (timeout)\n", VGA_COLOR_LIGHT_GREY);
    }
}

#define PULL_HOST IP(10, 0, 2, 2)
#define PULL_PORT 8080
#define PULL_SRC_PORT 49152
#define PULL_STAGE_SIZE 16384

static uint8_t pull_stage[PULL_STAGE_SIZE];

static uint32_t pull_parse_dec(const char **p) {
    uint32_t v = 0;
    const char *s = *p;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    *p = s;
    return v;
}

static int pull_req_wait(const char *req, udp_reply_t *reply) {
    int ok = 0;
    if (udp_request_start(PULL_HOST, PULL_SRC_PORT, PULL_PORT,
                          req, (uint16_t)strlen(req)) == 0) {
        int start = get_tick();
        while (get_tick() - start < 100) {
            process_rx_packets();
            if (udp_request_try_reply(reply) == 0) { ok = 1; break; }
        }
    }
    udp_request_finish();
    return ok;
}

static void cmd_listhostfiles(char *args, uint8_t color) {
    (void)args;

    udp_reply_t reply;
    if (!pull_req_wait("LIST", &reply)) {
        printc("LIST request timed out\n", VGA_COLOR_RED);
        return;
    }

    if (reply.payload[0] != 'O' || reply.payload[1] != 'K') {
        printc("Unexpected reply\n", VGA_COLOR_RED);
        return;
    }

    uint32_t pos = 0;
    uint32_t lines = 0;
    uint32_t entries = 0;
    while (pos < reply.payload_len && lines < 64) {
        uint32_t end = pos;
        while (end < reply.payload_len && reply.payload[end] != '\n') end++;

        if (lines > 0 && end > pos) {
            const char *ls = (const char *)reply.payload + pos;
            const char *sp = ls;
            while ((uint32_t)(sp - ls) < end - pos && *sp != ' ') sp++;

            if ((uint32_t)(sp - ls) < end - pos) {
                printc("  ", color);
                for (uint32_t k = pos; k < (uint32_t)(sp - (const char *)reply.payload); k++)
                    putchar(reply.payload[k], color);
                const char *sz = sp + 1;
                printc("  (", color);
                print_int((int)pull_parse_dec(&sz));
                printc(" bytes)\n", color);
                entries++;
            }
        }
        pos = end + 1;
        lines++;
    }

    if (entries == 0)
        printc("No files on host\n", color);
}

static void cmd_hostfilesize(char *args, uint8_t color) {
    unsigned char fname[32];
    get_arg(args, 0, (char*)fname, 32);

    if (fname[0] == '\0') {
        printc("Usage: hostfilesize <name>\n", VGA_COLOR_RED);
        return;
    }

    char req[64];
    snprintf(req, sizeof(req), "SIZE %s", fname);

    udp_reply_t reply;
    if (!pull_req_wait(req, &reply)) {
        printc("SIZE request timed out\n", VGA_COLOR_RED);
        return;
    }

    const char *p = (const char *)reply.payload;
    if (!starts_with(reply.payload, "OK")) {
        printc("File not found on host\n", VGA_COLOR_RED);
        return;
    }
    p += 2;
    while (*p == ' ') p++;
    uint32_t size = pull_parse_dec(&p);

    printc("Host size of ", color);
    printc((char*)fname, color);
    printc(": ", color);
    print_int(size);
    printc(" bytes\n", color);
}

static void cmd_pullhostfile(char *args, uint8_t color) {
    unsigned char remote[32];
    unsigned char local[32];

    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }

    get_arg(args, 0, (char*)remote, 32);
    get_arg(args, 1, (char*)local, 32);
    if (remote[0] == '\0') {
        printc("Usage: pull <name> [local-name]\n", VGA_COLOR_RED);
        return;
    }
    if (local[0] == '\0') {
        int i = 0;
        while (remote[i] && i < 31) { local[i] = remote[i]; i++; }
        local[i] = '\0';
    }

    char req[64];
    snprintf(req, sizeof(req), "SIZE %s", remote);

    udp_reply_t reply;
    if (!pull_req_wait(req, &reply)) {
        printc("SIZE request timed out\n", VGA_COLOR_RED);
        return;
    }

    uint32_t total = 0;
    const char *p = (const char *)reply.payload;
    if (starts_with(reply.payload, "OK")) {
        p += 2;
        while (*p == ' ') p++;
        total = pull_parse_dec(&p);
    } else {
        printc("Remote file not found\n", VGA_COLOR_RED);
        return;
    }

    printc("Pulling ", color);
    printc((char*)remote, color);
    printc(" (", color);
    print_int(total);
    printc(" bytes) -> ", color);
    printc((char*)local, color);
    printc("\n", color);

    if (fat32_create_file(fs, (char*)local, NULL, 0) != 0) {
        printc("Failed to create target file\n", VGA_COLOR_RED);
        return;
    }

    uint32_t off = 0;
    uint32_t prev_len = 0;
    uint32_t stage_off = 0;
    char stats[48];

    while (off < total) {
        snprintf(req, sizeof(req), "GET %s %u", remote, (unsigned)off);

        if (!pull_req_wait(req, &reply)) {
            printc("\nGET request timed out\n", VGA_COLOR_RED);
            return;
        }

        p = (const char *)reply.payload;
        if (!starts_with(reply.payload, "OK")) {
            printc("\nServer error on GET\n", VGA_COLOR_RED);
            return;
        }
        p += 2;
        while (*p == ' ') p++;
        uint32_t r_off = pull_parse_dec(&p);
        while (*p == ' ') p++;
        pull_parse_dec(&p);
        while (*p == ' ') p++;
        uint32_t r_len = pull_parse_dec(&p);

        int data_start = 0;
        while (data_start < reply.payload_len && reply.payload[data_start] != '\n')
            data_start++;
        data_start++;

        if (r_off != off) {
            printc("\nServer offset mismatch\n", VGA_COLOR_RED);
            return;
        }

        uint32_t avail = r_len;
        if (data_start + (int)avail > reply.payload_len)
            avail = (uint32_t)(reply.payload_len - data_start);

        if (avail == 0) break;

        for (uint32_t k = 0; k < avail; k++)
            pull_stage[stage_off + k] = reply.payload[data_start + k];
        stage_off += avail;

        if (stage_off >= PULL_STAGE_SIZE) {
            if (fat32_append_file(fs, (char*)local, pull_stage, stage_off) != 0) {
                printc("\nWrite failed (disk full?)\n", VGA_COLOR_RED);
                return;
            }
            stage_off = 0;
        }
        off += avail;

        uint32_t dots = (uint32_t)(((uint64_t)off * 50) / total);
        uint32_t pct  = (uint32_t)(((uint64_t)off * 100) / total);
        int slen = snprintf(stats, sizeof(stats), "%u/%u bytes (%u%%)",
                            (unsigned)off, (unsigned)total, (unsigned)pct);

        for (uint32_t k = 0; k < prev_len; k++)
            putchar('\b', color);
        printc("[", color);
        for (uint32_t k = 0; k < dots; k++)
            printc(".", color);
        printc("] ", color);
        printc(stats, color);
        prev_len = 1 + dots + 2 + (uint32_t)slen;
    }
    printc("\n", color);

    if (stage_off > 0 &&
        fat32_append_file(fs, (char*)local, pull_stage, stage_off) != 0) {
        printc("Write failed (disk full?)\n", VGA_COLOR_RED);
        return;
    }

    printc("Saved ", color);
    print_int(off);
    printc(" bytes to ", color);
    printc((char*)local, color);
    printc("\n", color);
}

static void cmd_dumpelf(char *args, uint8_t color) {
    unsigned char filename[32];
    get_arg(args, 0, (char*)filename, 32);

    if (filename[0] == '\0') {
        printf("Usage: dumpelf <filename>\n");
        return;
    }

    Buffer_t file = readfile(filename);
    dumpelf(file.bytes, file.size);
    kfree(file.bytes);
}

static void cmd_runelf(char *args, uint8_t color) {
    unsigned char filename[32];
    get_arg(args, 0, (char*)filename, 32);

    if (filename[0] == '\0') {
        printf("Usage: runelf <filename>\n");
        return;
    }

    Buffer_t file = readfile(filename);
    runelf(file);
    kfree(file.bytes);
}

static void cmd_processes(char *args, uint8_t color) {
    (void)args;
    printf("Processes count: %d\n", nr_processes);
}

static int streq(unsigned char *a, char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == (unsigned char)*b;
}

static const char* starts_with(const unsigned char* str, const char* prefix) {
    while (*prefix) {
        if (*str != (unsigned char)*prefix) return 0;
        str++; prefix++;
    }
    return (const char*)str;
}

void run_command(unsigned char *cmd_input, uint8_t color) {
    char *args = (char *)cmd_input;
    char *cmd_name_ptr = (char *)cmd_input;
    
    int len = 0;
    while (cmd_name_ptr[len] != ' ' && cmd_name_ptr[len] != '\0') {
        len++;
    }
    
    char name_buf[32];
    if (len >= 32) len = 31;
    for (int i = 0; i < len; i++) name_buf[i] = cmd_name_ptr[i];
    name_buf[len] = '\0';
    
    if (len < 32 && cmd_name_ptr[len] == ' ') {
        args = cmd_name_ptr + len + 1;
        while (*args == ' ') args++;
    } else {
        args = "";
    }

    for (int i = 0; i < num_commands; i++) {
        if (streq((unsigned char*)name_buf, commands[i].name)) {
            commands[i].func(args, color);
            return;
        }
    }

    if (strlen((char*)cmd_input) != 0)
        printc("Unknown command. Type 'help' for available commands\n", color);
}
