#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>

typedef struct {
    char *name;
    void (*func)(uint8_t color);
} Command;

// System
static void cmd_help(uint8_t color);
static void cmd_hello(uint8_t color);
static void cmd_contributors(uint8_t color);
static void cmd_clear(uint8_t color);
static void cmd_version(uint8_t color);
static void cmd_chars(uint8_t color);
static void cmd_uptime(uint8_t color);
static void cmd_meminfo(uint8_t color);
static void cmd_lspci(uint8_t color);

// Keyboard
static void cmd_setkeyswe(uint8_t color);
static void cmd_setkeyus(uint8_t color);
static void cmd_setkeyuk(uint8_t color);

// Timer / power
static void cmd_sleep5(uint8_t color);
static void cmd_print_ticks(uint8_t color);
static void cmd_reboot(uint8_t color);

// Scripting
static void cmd_gk(uint8_t color);
static void cmd_gk_run_file(const char* filename, uint8_t color);

// Filesystem
static void cmd_fsmount(uint8_t color);
static void cmd_ls(uint8_t color);
static void cmd_cat(uint8_t color);
static void cmd_fsinfo(uint8_t color);
static void cmd_touch(uint8_t color);
static void cmd_rm(uint8_t color);
static void cmd_cp(uint8_t color);
static void cmd_mv(uint8_t color);
static void cmd_mkdir(uint8_t color);
static void cmd_echo(uint8_t color);
static void cmd_write(uint8_t color);

// Network
static void cmd_ping(uint8_t color);

// Dispatcher
static int streq(unsigned char *a, char *b);
void run_command(unsigned char *input, uint8_t color);

#endif