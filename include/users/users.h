//ember2819
// User system
#ifndef USERS_H
#define USERS_H

#include <stdint.h>

#define RING_KERNEL  0   // kernel-internal, never assigned to a login session
#define RING_ADMIN   1   // root / superuser
#define RING_USER    3   // normal user
#define PERM_FS_READ    (1 << 0)   // read files from the filesystem
#define PERM_FS_WRITE   (1 << 1)   // write / create / delete files
#define PERM_FS_EXEC    (1 << 2)   // execute .gk scripts
#define PERM_USER_MGMT  (1 << 3)   // add / remove / passwd users
#define PERM_SYS_CTRL   (1 << 4)   // reboot, halt, hardware commands
#define PERM_ALL        0xFF       // superuser gets everything

#define PERMS_ADMIN  (PERM_ALL)
#define PERMS_USER   (PERM_FS_READ | PERM_FS_EXEC)

#define MAX_USERS       8
#define MAX_USERNAME    16
#define MAX_PASSWORD    32

typedef struct {
    char     name[MAX_USERNAME];
    char     password[MAX_PASSWORD];  // ATTENTION: Stored in plaintext we should update that
    uint8_t  ring;         
    uint32_t perms;  
    uint8_t  active; 
} user_t;

typedef struct {
    user_t  *user;
    uint8_t  ring;
    uint32_t perms;   // effective permissions
} session_t;

void users_init(void);

// Try to login
int  users_login(const char *name, const char *password);

// Log out the current session
void users_logout(void);

user_t *users_current(void);

uint8_t users_current_ring(void);

int users_has_perm(uint32_t required_perms);

// Add a new user
// Returns 0 on success, -1 on failure.
int users_add(const char *name, const char *password, uint8_t ring);

// Delete a user by name
// Returns 0 on success, -1 on failure.
int users_del(const char *name);

// Change a user's password.
// Returns 0 on success, -1 on failure.
int users_passwd(const char *name, const char *new_password);

// List all users
void users_list(uint8_t color);

// Switch user (su).  Prompts for password internally.
// Returns 1 on success, 0 on failure.
int users_su(const char *name, const char *password);

#endif // USERS_H
