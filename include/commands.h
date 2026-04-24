#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>

// MorganPG1 - Moved some declarations from commands.c to commands.h
typedef struct {
    char *name;
    void (*func)(uint8_t color);
} Command;

// ---- Original commands ----
static void cmd_help(uint8_t color);
static void cmd_hello(uint8_t color);
static void cmd_contributors(uint8_t color);
static void cmd_setkeyswe(uint8_t color);
static void cmd_setkeyus(uint8_t color);
static void cmd_setkeyuk(uint8_t color);

//ember2819:clear
static void cmd_clear(uint8_t color);

//TheOtterMonarch:version
static void cmd_version(uint8_t color);
static void cmd_chars(uint8_t color);

//ember2819
static void cmd_gk(uint8_t color);

//Pumpkicks: timer and reboot
static void cmd_sleep5(uint8_t color);
static void cmd_print_ticks(uint8_t color);
static void cmd_reboot(uint8_t color);

static void cmd_fsmount(uint8_t color);
static void cmd_ls(uint8_t color);
static void cmd_cat(uint8_t color);
static void cmd_fsinfo(uint8_t color);
static void cmd_touch(uint8_t color);

//ember2819
static void cmd_gk_run_file(const char* filename, uint8_t color);

// User / permission management
static void cmd_whoami(uint8_t color);
static void cmd_users(uint8_t color);
static void cmd_useradd(uint8_t color);
static void cmd_userdel(uint8_t color);
static void cmd_passwd(uint8_t color);
static void cmd_su(uint8_t color);
static void cmd_logout(uint8_t color);

// Filesystem utilities
static void cmd_rm(uint8_t color);
static void cmd_cp(uint8_t color);
static void cmd_mv(uint8_t color);
static void cmd_mkdir(uint8_t color);
static void cmd_echo(uint8_t color);
static void cmd_write(uint8_t color);

// System info
static void cmd_uptime(uint8_t color);
static void cmd_meminfo(uint8_t color);

// Dispatcher
static int streq(unsigned char *a, char *b);
void run_command(unsigned char *input, uint8_t color);

void do_login_prompt(void);

#endif
