#include "commands.h"
#include "bootoptions.h"
#include "colors.h"
#include "drivers/keyboard.h"
#include "drivers/tables/timer/timer.h"
#include "layouts/kb_layouts.h"
#include "terminal/terminal.h"
#include "gk/gk.h"
#include <mem/mem.h>
#include "drivers/ata.h"
#include "fs/fs.h"
#include "fs/fat16.h"
#include "users/users.h"
#include <stdint.h>
#define PERM_DENIED(color) \
    do { printc("\nPermission denied.\n", VGA_COLOR_RED); return; } while(0)

#define REQUIRE_LOGIN(color) \
    do { if (!users_current()) { printc("\nNot logged in.\n", VGA_COLOR_RED); return; } } while(0)

#define REQUIRE_PERM(perm, color) \
    do { if (!users_has_perm(perm)) PERM_DENIED(color); } while(0)

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
    // --- user management ---
    { "whoami",       cmd_whoami       },
    { "users",        cmd_users        },
    { "useradd",      cmd_useradd      },
    { "userdel",      cmd_userdel      },
    { "passwd",       cmd_passwd       },
    { "su",           cmd_su           },
    { "logout",       cmd_logout       },
};

static int num_commands = sizeof(commands) / sizeof(commands[0]);

static struct drive_fs_t *fs;

static void cmd_help(uint8_t color) {
    printc("\n--- System ---\n", VGA_COLOR_LIGHT_CYAN);
    printc("help        - Show this message\n", color);
    printc("hello       - Say hello\n", color);
    printc("contributors- List contributors\n", color);
    printc("clear       - Clear the screen\n", color);
    printc("version     - OS version\n", color);
    printc("chars       - Print available characters\n", color);
    printc("uptime      - Show system uptime (ticks)\n", color);
    printc("meminfo     - Show memory info\n", color);

    printc("\n--- Keyboard ---\n", VGA_COLOR_LIGHT_CYAN);
    printc("setkeyswe   - Swedish QWERTY layout\n", color);
    printc("setkeyus    - US QWERTY layout\n", color);
    printc("setkeyuk    - UK QWERTY layout\n", color);

    printc("\n--- Timer / Power ---\n", VGA_COLOR_LIGHT_CYAN);
    printc("sleep       - Sleep 5 seconds\n", color);
    printc("ticks       - Print timer tick count\n", color);
    printc("reboot      - Reboot  [admin]\n", color);

    printc("\n--- Scripting ---\n", VGA_COLOR_LIGHT_CYAN);
    printc("gk          - GK scripting language (demo)\n", color);
    printc("gk <file>   - Run a .gk script from the FS\n", color);

    printc("\n--- Filesystem ---\n", VGA_COLOR_LIGHT_CYAN);
    printc("fsmount     - Mount the FAT16 filesystem  [admin]\n", color);
    printc("ls          - List root directory\n", color);
    printc("cat         - Print a file's contents\n", color);
    printc("fsinfo      - Filesystem volume info\n", color);
    printc("touch       - Create a file (prompts)  [write]\n", color);
    printc("rm          - Delete a file             [admin]\n", color);
    printc("cp          - Copy a file               [write]\n", color);
    printc("mv          - Move/rename a file        [write]\n", color);
    printc("mkdir       - Create a directory        [write]\n", color);
    printc("echo        - Print text to screen\n", color);
    printc("write       - Append text to a file     [write]\n", color);

    printc("\n--- Users ---\n", VGA_COLOR_LIGHT_CYAN);
    printc("whoami      - Show current user\n", color);
    printc("users       - List all users            [admin]\n", color);
    printc("useradd     - Add a user                [admin]\n", color);
    printc("userdel     - Delete a user             [admin]\n", color);
    printc("passwd      - Change a password\n", color);
    printc("su          - Switch user\n", color);
    printc("logout      - Log out of current session\n", color);
    printc("\n", color);
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
    printc("\nGeckoOS v1.1\nUsing GeckoOS Bootloader 1.0\n", color);
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
    REQUIRE_LOGIN(color);
    REQUIRE_PERM(PERM_SYS_CTRL, color);
    print("\nRebooting...");
    reboot();
}

static void cmd_print_ticks(uint8_t color) {
    print("\nTick: ");
    print_int(get_tick());
    print("\n");
}

static void cmd_fsmount(uint8_t color) {
    REQUIRE_LOGIN(color);
    REQUIRE_PERM(PERM_SYS_CTRL, color);
    printc("\n", color);
    if (!get_kdrive(1)) {
        printc("No slave drive found. Is fat16.img attached as a second drive?\n", VGA_COLOR_RED);
        return;
    }
    fs = fs_drive_open(get_kdrive(1));
    if (fs == 0) {
        printc("Filesystem mount failed. Is fat16.img a valid FAT16 image?\n", VGA_COLOR_RED);
        return;
    }
    printc("Filesystem mounted successfully.\n", color);
}

static void cmd_ls(uint8_t color) {
    REQUIRE_LOGIN(color);
    REQUIRE_PERM(PERM_FS_READ, color);
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
    REQUIRE_LOGIN(color);
    REQUIRE_PERM(PERM_FS_READ, color);
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
    REQUIRE_LOGIN(color);
    REQUIRE_PERM(PERM_FS_READ, color);
    if (!fs) {
        printc("\nFilesystem not mounted. Run 'fsmount' first.\n", VGA_COLOR_RED);
        return;
    }
    printc("\n", color);
    fat16_print_info(fs, color);
}

static void cmd_touch(uint8_t color) {
    REQUIRE_LOGIN(color);
    REQUIRE_PERM(PERM_FS_WRITE, color);
    unsigned char fname[32];
    unsigned char content[256];

    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }

    printc("\nFilename: ", color);
    input(fname, 32, color);
    printc("\nContent (single line): ", color);
    input(content, 255, color);
    printc("\n", color);

    int result = fat16_create_file(fs, (char*)fname,
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
    REQUIRE_LOGIN(color);
    REQUIRE_PERM(PERM_USER_MGMT, color);
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

    int result = fat16_delete_file(fs, (char*)fname);
    if (result == 0) {
        printc("Deleted: ", color);
        printc((char*)fname, color);
        printc("\n", color);
    } else {
        printc("Failed to delete (file not found or FS error).\n", VGA_COLOR_RED);
    }
}

static void cmd_cp(uint8_t color) {
    REQUIRE_LOGIN(color);
    REQUIRE_PERM(PERM_FS_WRITE, color);
    unsigned char src[32], dst[32];
    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }

    printc("\nSource filename: ", color);
    input(src, 32, color);
    printc("\nDestination filename: ", color);
    input(dst, 32, color);
    printc("\n", color);

    // Read source
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

    int result = fat16_create_file(fs, (char*)dst, copybuf, total);
    if (result == 0) {
        printc("Copied to: ", color);
        printc((char*)dst, color);
        printc("\n", color);
    } else {
        printc("Copy failed.\n", VGA_COLOR_RED);
    }
}

static void cmd_mv(uint8_t color) {
    REQUIRE_LOGIN(color);
    REQUIRE_PERM(PERM_FS_WRITE, color);
    unsigned char src[32], dst[32];
    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }

    printc("\nSource filename: ", color);
    input(src, 32, color);
    printc("\nDestination filename: ", color);
    input(dst, 32, color);
    printc("\n", color);

    // Read source content
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

    int rc = fat16_create_file(fs, (char*)dst, mvbuf, total);
    if (rc != 0) { printc("Move failed (could not create dst).\n", VGA_COLOR_RED); return; }

    fat16_delete_file(fs, (char*)src);
    printc("Moved: ", color);
    printc((char*)src, color);
    printc(" -> ", color);
    printc((char*)dst, color);
    printc("\n", color);
}

static void cmd_mkdir(uint8_t color) {
    REQUIRE_LOGIN(color);
    REQUIRE_PERM(PERM_FS_WRITE, color);
    unsigned char dname[32];
    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }

    printc("\nDirectory name: ", color);
    input(dname, 32, color);
    printc("\n", color);

    int result = fat16_mkdir(fs, (char*)dname);
    if (result == 0) {
        printc("Directory created: ", color);
        printc((char*)dname, color);
        printc("\n", color);
    } else {
        printc("Failed to create directory.\n", VGA_COLOR_RED);
    }
}

static void cmd_echo(uint8_t color) {
    // No permission needed – pure output
    unsigned char msg[256];
    printc("\n", color);
    input(msg, 256, color);
    printc("\n", color);
    printc((char*)msg, color);
    printc("\n", color);
}

static void cmd_write(uint8_t color) {
    REQUIRE_LOGIN(color);
    REQUIRE_PERM(PERM_FS_WRITE, color);
    unsigned char fname[32];
    unsigned char content[256];
    if (!fs) { kprintf(SEVERITY_WARNING, "Not mounted\n"); return; }

    printc("\nFilename: ", color);
    input(fname, 32, color);
    printc("\nText to append: ", color);
    input(content, 255, color);
    printc("\n", color);

    int result = fat16_append_file(fs, (char*)fname,
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
    uint32_t seconds = ticks / 50; // timer is set to 50 Hz in kernel.c
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
    printc("  (exact used bytes depend on runtime allocations)\n", color);
    printc("  Total RAM  : detected via BIOS e820 (not yet parsed)\n", color);
    printc("\n", color);
}

static void cmd_whoami(uint8_t color) {
    user_t *u = users_current();
    if (!u) {
        printc("\nNot logged in.\n", VGA_COLOR_RED);
        return;
    }
    printc("\n", color);
    printc(u->name, color);
    if (u->ring == RING_ADMIN)
        printc("  (admin / ring 1)\n", VGA_COLOR_LIGHT_RED);
    else
        printc("  (user / ring 3)\n", color);
}

static void cmd_users(uint8_t color) {
    REQUIRE_LOGIN(color);
    REQUIRE_PERM(PERM_USER_MGMT, color);
    users_list(color);
}

static void cmd_useradd(uint8_t color) {
    REQUIRE_LOGIN(color);
    REQUIRE_PERM(PERM_USER_MGMT, color);

    unsigned char uname[MAX_USERNAME];
    unsigned char upass[MAX_PASSWORD];
    unsigned char urole[4];

    printc("\nNew username: ", color);
    input(uname, MAX_USERNAME, color);
    printc("\nPassword: ", color);
    input(upass, MAX_PASSWORD, color);
    printc("\nRole (0=admin, 1=user): ", color);
    input(urole, 4, color);
    printc("\n", color);

    uint8_t ring = (urole[0] == '0') ? RING_ADMIN : RING_USER;

    int rc = users_add((char*)uname, (char*)upass, ring);
    if (rc == 0) {
        printc("User added: ", color);
        printc((char*)uname, color);
        printc("\n", color);
    } else {
        printc("Failed (name exists or table full).\n", VGA_COLOR_RED);
    }
}

static void cmd_userdel(uint8_t color) {
    REQUIRE_LOGIN(color);
    REQUIRE_PERM(PERM_USER_MGMT, color);

    unsigned char uname[MAX_USERNAME];
    printc("\nUsername to delete: ", color);
    input(uname, MAX_USERNAME, color);
    printc("\n", color);

    int rc = users_del((char*)uname);
    if (rc == 0) {
        printc("User deleted: ", color);
        printc((char*)uname, color);
        printc("\n", color);
    } else {
        printc("Failed (not found, is root, or is current user).\n", VGA_COLOR_RED);
    }
}

static void cmd_passwd(uint8_t color) {
    REQUIRE_LOGIN(color);

    unsigned char uname[MAX_USERNAME];
    unsigned char upass[MAX_PASSWORD];

    printc("\nUsername: ", color);
    input(uname, MAX_USERNAME, color);
    printc("\nNew password: ", color);
    input(upass, MAX_PASSWORD, color);
    printc("\n", color);

    int rc = users_passwd((char*)uname, (char*)upass);
    if (rc == 0) {
        printc("Password updated.\n", color);
    } else {
        printc("Failed (user not found or permission denied).\n", VGA_COLOR_RED);
    }
}

static void cmd_su(uint8_t color) {
    unsigned char uname[MAX_USERNAME];
    unsigned char upass[MAX_PASSWORD];

    printc("\nUsername: ", color);
    input(uname, MAX_USERNAME, color);
    printc("\nPassword: ", color);
    input(upass, MAX_PASSWORD, color);
    printc("\n", color);

    int ok = users_su((char*)uname, (char*)upass);
    if (ok) {
        printc("Switched to: ", color);
        printc((char*)uname, color);
        printc("\n", color);
    } else {
        printc("Invalid credentials.\n", VGA_COLOR_RED);
    }
}

//ember2819: logout
static void cmd_logout(uint8_t color) {
    if (!users_current()) {
        printc("\nNot logged in.\n", VGA_COLOR_RED);
        return;
    }
    printc("\nLogged out.\n", color);
    users_logout();
    do_login_prompt();
}

static GkState gk_state;

static void cmd_gk(uint8_t color) {
    printc("\nGeckoOS scripting language is running!\n", color);
}

static void cmd_gk_run_file(const char* filename, uint8_t color) {
    REQUIRE_LOGIN(color);
    REQUIRE_PERM(PERM_FS_EXEC, color);

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

//login prmopt
void do_login_prompt(void) {
    unsigned char uname[MAX_USERNAME];
    unsigned char upass[MAX_PASSWORD];

    while (1) {
        printc("\n--- GeckoOS Login ---\n", VGA_COLOR_LIGHT_CYAN);
        printc("Username: ", PROMPT_COLOR);
        input(uname, MAX_USERNAME, TERM_COLOR);
        printc("\nPassword: ", PROMPT_COLOR);
        input(upass, MAX_PASSWORD, TERM_COLOR);
        printc("\n", TERM_COLOR);

        if (users_login((char*)uname, (char*)upass)) {
            printc("Welcome, ", VGA_COLOR_LIGHT_GREEN);
            printc((char*)uname, VGA_COLOR_LIGHT_GREEN);
            printc("!\n", VGA_COLOR_LIGHT_GREEN);
            return;
        } else {
            printc("Invalid username or password.\n", VGA_COLOR_RED);
        }
    }
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
    // "gk <filename>" shorthand
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
