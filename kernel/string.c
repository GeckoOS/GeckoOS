#include "string.h"

static void strcpy(char* d, const char* s) {
    int i = 0; while (s[i]) { d[i] = s[i]; i++; } d[i] = 0;
}
static bool strcmp(const char* a, const char* b) {
    int i = 0;
    while (a[i] && b[i]) { if (a[i] != b[i]) return false; i++; }
    return a[i] == b[i];
}
static int strlen(const char* s) {
    int n = 0; while (s[n]) n++; return n;
}
