#include <stdint.h>
void* memcpy(void* dest, const void* src, unsigned long n) {
    // n = Number of bytes

    unsigned char* d = dest;
    const unsigned char* s = src;

    // Iterate n times, copying the byte in s into the same index in d
    for (unsigned long i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dest;
}

// [Ember2819: BEGIN - memset implementation]
void* memset(void* dest, int val, unsigned long n) {
    unsigned char* d = dest;
    for (unsigned long i = 0; i < n; i++) {
        d[i] = (unsigned char)val;
    }
    return dest;
}
// [Ember2819: END]

// Pumpkicks
int strlen(char* ptr) {
    int i = 0;
    while (ptr[i]) i++;
    return i;
}

//replace with real allocator later but should be fine for now
extern unsigned char __bss_start;
extern unsigned char __bss_end;
static unsigned long heap_ptr = 0;

void* malloc(unsigned long size) {
    if ((heap_ptr + size) > __bss_end) {
        return 0;
    }
    void* p = (void*)(uintptr_t)(&__bss_start)[heap_ptr];
    heap_ptr += size;
    return p;
}
