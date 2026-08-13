#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>

typedef struct {
    char *name;
    void (*func)(char *args, uint8_t color);
} Command;

// System
static void cmd_help(char *args, uint8_t color);
static void cmd_hello(char *args, uint8_t color);
static void cmd_contributors(char *args, uint8_t color);
static void cmd_clear(char *args, uint8_t color);
static void cmd_version(char *args, uint8_t color);
static void cmd_chars(char *args, uint8_t color);
static void cmd_uptime(char *args, uint8_t color);
static void cmd_meminfo(char *args, uint8_t color);
static void cmd_lspci(char *args, uint8_t color);

// Keyboard
static void cmd_setkeyswe(char *args, uint8_t color);
static void cmd_setkeyus(char *args, uint8_t color);
static void cmd_setkeyuk(char *args, uint8_t color);

// Timer / power
static void cmd_sleep5(char *args, uint8_t color);
static void cmd_print_ticks(char *args, uint8_t color);
static void cmd_reboot(char *args, uint8_t color);

// Scripting
static void cmd_gk(char *args, uint8_t color);
static void cmd_gk_run_file(const char* filename, uint8_t color);

// Filesystem
static void cmd_fsmount(char *args, uint8_t color);
static void cmd_ls(char *args, uint8_t color);
static void cmd_cat(char *args, uint8_t color);
static void cmd_fsinfo(char *args, uint8_t color);
static void cmd_touch(char *args, uint8_t color);
static void cmd_rm(char *args, uint8_t color);
static void cmd_cp(char *args, uint8_t color);
static void cmd_mv(char *args, uint8_t color);
static void cmd_mkdir(char *args, uint8_t color);
static void cmd_echo(char *args, uint8_t color);
static void cmd_write(char *args, uint8_t color);
static void cmd_dumpelf(char *args, uint8_t color);
static void cmd_runelf(char *args, uint8_t color);

// Network
static void cmd_ping(char *args, uint8_t color);

// Processes
static void cmd_processes(char *args, uint8_t color);

// Dispatcher
static int streq(unsigned char *a, char *b);
void run_command(unsigned char *input, uint8_t color);

#endif
