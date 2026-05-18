//ember2819
#include <users/users.h>
#include <mem.h>
#include <terminal/terminal.h>
#include <drivers/vga.h>
#include <stdint.h>

static user_t    user_table[MAX_USERS];
static int       user_count = 0;
static session_t current_session = { 0, RING_KERNEL, 0 };

// Simple strcmp for our kernel, should be libc but im lazy
static int k_strcmp(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return (unsigned char)*a - (unsigned char)*b;
        a++; b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

// Simple strcpy
static void k_strcpy(char *dst, const char *src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static user_t *find_user(const char *name) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (user_table[i].active && k_strcmp(user_table[i].name, name) == 0)
            return &user_table[i];
    }
    return 0;
}

static user_t *find_empty_slot(void) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (!user_table[i].active)
            return &user_table[i];
    }
    return 0;
}

void users_init(void) {
    memset(user_table, 0, sizeof(user_table));
    user_count = 0;

    user_t *root = &user_table[0];
    root->active = 1;
    k_strcpy(root->name,     "root",  MAX_USERNAME);
    k_strcpy(root->password, "root",  MAX_PASSWORD);
    root->ring  = RING_ADMIN;
    root->perms = PERMS_ADMIN;
    user_count++;

    user_t *guest = &user_table[1];
    guest->active = 1;
    k_strcpy(guest->name,     "guest", MAX_USERNAME);
    k_strcpy(guest->password, "",      MAX_PASSWORD);
    guest->ring  = RING_USER;
    guest->perms = PERMS_USER;
    user_count++;

    current_session.user  = 0;
    current_session.ring  = RING_KERNEL;
    current_session.perms = 0;
}

int users_login(const char *name, const char *password) {
    user_t *u = find_user(name);
    if (!u) return 0;
    if (k_strcmp(u->password, password) != 0) return 0;

    current_session.user  = u;
    current_session.ring  = u->ring;
    current_session.perms = u->perms;
    return 1;
}

void users_logout(void) {
    current_session.user  = 0;
    current_session.ring  = RING_KERNEL;
    current_session.perms = 0;
}

user_t *users_current(void) {
    return current_session.user;
}

uint8_t users_current_ring(void) {
    return current_session.ring;
}

int users_has_perm(uint32_t required_perms) {
    if (!current_session.user) return 0;
    return (current_session.perms & required_perms) == required_perms;
}

int users_add(const char *name, const char *password, uint8_t ring) {
    if (!users_has_perm(PERM_USER_MGMT)) return -1;

    // Name must not already exist
    if (find_user(name)) return -1;

    user_t *slot = find_empty_slot();
    if (!slot) return -1;   // table full

    slot->active = 1;
    k_strcpy(slot->name,     name,     MAX_USERNAME);
    k_strcpy(slot->password, password, MAX_PASSWORD);
    slot->ring  = ring;
    slot->perms = (ring == RING_ADMIN) ? PERMS_ADMIN : PERMS_USER;
    user_count++;
    return 0;
}

int users_del(const char *name) {
    if (!users_has_perm(PERM_USER_MGMT)) return -1;

    // Cannot delete root: no rm -rf
    if (k_strcmp(name, "root") == 0) return -1;

    // Cannot delete self
    if (current_session.user && k_strcmp(current_session.user->name, name) == 0)
        return -1;

    user_t *u = find_user(name);
    if (!u) return -1;

    memset(u, 0, sizeof(user_t));
    user_count--;
    return 0;
}

int users_passwd(const char *name, const char *new_password) {
    user_t *target = find_user(name);
    if (!target) return -1;

    // Admins can change anyone's password
    if (!users_has_perm(PERM_USER_MGMT)) {
        if (!current_session.user) return -1;
        if (k_strcmp(current_session.user->name, name) != 0) return -1;
    }

    k_strcpy(target->password, new_password, MAX_PASSWORD);
    return 0;
}

void users_list(uint8_t color) {
    printc("\n--- Users ---\n", color);
    for (int i = 0; i < MAX_USERS; i++) {
        if (!user_table[i].active) continue;
        printc("  ", color);
        printc(user_table[i].name, color);
        if (user_table[i].ring == RING_ADMIN)
            printc("  [admin]", VGA_COLOR_LIGHT_RED);
        else
            printc("  [user] ", color);
        printc("\n", color);
    }
    printc("\n", color);
}

int users_su(const char *name, const char *password) {
    user_t *u = find_user(name);
    if (!u) return 0;
    if (k_strcmp(u->password, password) != 0) return 0;

    current_session.user  = u;
    current_session.ring  = u->ring;
    current_session.perms = u->perms;
    return 1;
}
