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
#include <drivers/e1000.h>

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
    // --- keyboard ---
    { "setkeyswe",    cmd_setkeyswe    },
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
    // --- network ---
    { "ping",         cmd_ping         },
};

static int num_commands = sizeof(commands) / sizeof(commands[0]);

static struct drive_fs_t *fs;

static const char* help_lines[] = {
    "--- System ---",
    "help        - Show this message",
    "hello       - Say hello",
    "contributors- List contributors",
    "clear       - Clear the screen",
    "version     - OS version",
    "chars       - Print available characters",
    "uptime      - Show system uptime (ticks)",
    "meminfo     - Show memory info",
    "lspci       - List PCI devices",
    "",
    "--- Keyboard ---",
    "setkeyswe   - Swedish QWERTY layout",
    "setkeyus    - US QWERTY layout",
    "setkeyuk    - UK QWERTY layout",
    "",
    "--- Timer / Power ---",
    "sleep       - Sleep 5 seconds",
    "ticks       - Print timer tick count",
    "reboot      - Reboot",
    "",
    "--- Scripting ---",
    "gk          - GK scripting language (demo)",
    "gk <file>   - Run a .gk script from the FS",
    "",
    "--- FAT32 Filesystem ---",
    "fsmount     - Mount the FAT32 data disk (fat32.img)",
    "ls          - List files in root directory",
    "cat         - Read and display a file",
    "fsinfo      - Show volume info (label, size, clusters)",
    "touch       - Create a new file with content",
    "rm          - Delete a file",
    "cp          - Copy a file to a new name",
    "mv          - Move/rename a file",
    "mkdir       - Create a new directory",
    "echo        - Print text to screen",
    "write       - Append text to an existing file",
    "",
    "--- Network ---",
    "ping <ip>   - Ping an IP address (e.g. ping 10.0.2.2)",
    "",
    0
};

#define SPACE_SC 0x39

static void cmd_help(uint8_t color) {
    int num_lines = 0;
    while (help_lines[num_lines] != 0) num_lines++;

    int rows = 20;
    int pages = (num_lines + rows - 1) / rows;
    int page = 0;

    while (1) {
        terminal_clear(color);
        int start = page * rows;
        int end = start + rows;
        if (end > num_lines) end = num_lines;

        for (int i = start; i < end; i++) {
            if (help_lines[i][0] == '-') {
                printc((char*)help_lines[i], VGA_COLOR_LIGHT_CYAN);
            } else if (help_lines[i][0] == '\0') {
                printc("\n", color);
            } else {
                printc((char*)help_lines[i], color);
            }
            printc("\n", color);
        }

        if (pages > 1) {
            printc("\n", color);
            printc("[Space: next page  Enter: exit]  Page ", VGA_COLOR_DARK_GREY);
            print_int(page + 1);
            printc("/", VGA_COLOR_DARK_GREY);
            print_int(pages);
            printc("\n", color);
        }

        while (1) {
            scancode_t sc = ps2_kb_wfi();
            if (sc & 0x80) continue;
            if (sc == 0) continue;
            if (sc == ENTER_SC) return;
            if (pages <= 1) return;   /* single page: any key exits */
            if (sc == SPACE_SC) {
                page = (page + 1) % pages;
                break;
            }
        }
    }
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
    printc("DarkThemeGeek\n", color);
    printc("nfoxers\n", color);
}

static void cmd_setkeyswe(uint8_t color) {
    set_layout(LAYOUTS[1]);
    printc("\nKeyboard layout set to Swedish QWERTY\n", color);
}

static void cmd_setkeyus(uint8_t color) {
    set_layout(LAYOUTS[0]);
    printc("\nKeyboard layout set to US QWERTY\n", color);
}

static void cmd_setkeyuk(uint8_t color) {
    set_layout(LAYOUTS[2]);
    printc("\nKeyboard layout set to UK QWERTY\n", color);
}

static void cmd_clear(uint8_t color) {
    terminal_clear(color);
}

static void cmd_version(uint8_t color) {
    printc("\nGeckoOS v2.0 (GRUB/Multiboot2)\n", color);
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
        if ((i+1)%16 == 0) printc("\n", color);
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
    printc("\n", color);
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

static void cmd_ls(uint8_t color) {
    struct fs_entries_t entries;
    int i;
    print("\n");
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

static void cmd_cat(uint8_t color) {
    struct fs_entries_t entries;
    unsigned char fname[32];
    int i, found;

    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }

    printc("\nEnter filename: ", color);
    input(fname, 32, color);
    printc("\n", color);

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

static void cmd_fsinfo(uint8_t color) {
    if (!fs) {
        printc("\nFilesystem not mounted. Run 'fsmount' first.\n", VGA_COLOR_RED);
        return;
    }
    printc("\n", color);
    fat32_print_info(fs, color);
}

static void cmd_touch(uint8_t color) {
    unsigned char fname[32];
    unsigned char content[256];

    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }

    printc("\nFilename: ", color);
    input(fname, 32, color);
    printc("\nContent (single line): ", color);
    input(content, 255, color);
    printc("\n", color);

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

static void cmd_rm(uint8_t color) {
    unsigned char fname[32];
    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }

    printc("\nFilename to delete: ", color);
    input(fname, 32, color);
    printc("\n", color);

    printc("Are you sure? (y/n): ", VGA_COLOR_LIGHT_RED);
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

static void cmd_cp(uint8_t color) {
    unsigned char src[32], dst[32];
    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }

    printc("\nSource filename: ", color);
    input(src, 32, color);
    printc("\nDestination filename: ", color);
    input(dst, 32, color);
    printc("\n", color);

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

static void cmd_mv(uint8_t color) {
    unsigned char src[32], dst[32];
    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }

    printc("\nSource filename: ", color);
    input(src, 32, color);
    printc("\nDestination filename: ", color);
    input(dst, 32, color);
    printc("\n", color);

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

static void cmd_mkdir(uint8_t color) {
    unsigned char dname[32];
    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }

    printc("\nDirectory name: ", color);
    input(dname, 32, color);
    printc("\n", color);

    int result = fat32_mkdir(fs, (char*)dname);
    if (result == 0) {
        printc("Directory created: ", color);
        printc((char*)dname, color);
        printc("\n", color);
    } else {
        printc("Failed to create directory.\n", VGA_COLOR_RED);
    }
}

static void cmd_echo(uint8_t color) {
    unsigned char msg[256];
    printc("\n", color);
    input(msg, 256, color);
    printc("\n", color);
    printc((char*)msg, color);
    printc("\n", color);
}

static void cmd_write(uint8_t color) {
    unsigned char fname[32];
    unsigned char content[256];
    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }

    printc("\nFilename: ", color);
    input(fname, 32, color);
    printc("\nText to append: ", color);
    input(content, 255, color);
    printc("\n", color);

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

static void cmd_uptime(uint8_t color) {
    uint32_t ticks = get_tick();
    uint32_t seconds = ticks / 50;
    uint32_t minutes = seconds / 60;
    uint32_t hours   = minutes / 60;
    seconds %= 60;
    minutes %= 60;

    printc("\nUptime: ", color);
    print_int(hours);
    printc("h ", color);
    print_int(minutes);
    printc("m ", color);
    print_int(seconds);
    printc("s  (", color);
    print_int(ticks);
    printc(" ticks)\n", color);
}

static void cmd_meminfo(uint8_t color) {
    printc("\nMemory:\n", color);
    printc("  Heap base : 0x200000\n", color);
    printc("  Heap end  : 0x500000 (3 MB window, hardcoded)\n", color);
    printc("  TODO: wire up Multiboot2 memory map (Phase 1)\n", color);
    printc("\n", color);
}

static GkState gk_state;

static void cmd_gk(uint8_t color) {
    printc("\nGeckoOS scripting language is running!\n", color);
}

static void cmd_gk_run_file(const char* filename, uint8_t color) {
    if (!fs) {
        printc("\nFilesystem not mounted. Run 'fsmount' first.\n", VGA_COLOR_RED);
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
        printc("\nFile not found: ", VGA_COLOR_RED);
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

    printc("\n", color);
    gk_init(&gk_state);
    gk_run(&gk_state, src_buf);
}

static void cmd_lspci(uint8_t color) {
    printf("\n");
    (void)color;
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

static void cmd_ping(uint8_t color) {
    (void)color;

    printc("\nUsage: ping <ip> (e.g. ping 10.0.2.2)\n", VGA_COLOR_LIGHT_GREY);
    printc("Our IP: ", VGA_COLOR_LIGHT_CYAN);
    print_ip(net_ip);
    printc("\n", VGA_COLOR_LIGHT_GREY);

    unsigned char ip_str[32];
    printc("\nEnter IP to ping: ", color);
    input(ip_str, 32, color);
    printc("\n", color);

    if (strlen((char *)ip_str) == 0) {
        printc("No IP specified.\n", VGA_COLOR_RED);
        return;
    }

    uint32_t target_ip = parse_ip((char *)ip_str);
    printc("Pinging ", color);
    print_ip(target_ip);
    printc(" ...\n", color);

    arp_request(target_ip);

    int tick_start = get_tick();
    int resolved = 0;
    uint8_t mac[6];

    while (get_tick() - tick_start < 100) {
        process_rx_packets();
        if (arp_resolve(target_ip, mac)) {
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
    const char* after_gk = starts_with(cmd_input, "gk ");
    if (after_gk) {
        while (*after_gk == ' ') after_gk++;
        if (*after_gk != '\0') {
            cmd_gk_run_file(after_gk, color);
            return;
        }
    }

    for (int i = 0; i < num_commands; i++) {
        if (streq(cmd_input, commands[i].name)) {
            commands[i].func(color);
            return;
        }
    }

    if (strlen((char*)cmd_input) != 0)
        printc("\nUnknown command. Type 'help' for available commands\n", color);
    else
        printc("\n", color);
}