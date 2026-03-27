#include "terminal/terminal.h"
#include <stddef.h>
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

int strlen(char* ptr) {
    int i = 0;
    while (ptr[i]) i++;
    return i;
}

//replace with real allocator later but should be fine for now
static unsigned char heap[65536];
static unsigned long heap_ptr = 0;

void* malloc(unsigned long size) {
    if (heap_ptr + size > sizeof(heap)) {
        return 0;
    }
    void* p = &heap[heap_ptr];
    heap_ptr += size;
    return p;
}

extern uint32_t end;
uint32_t placement_address = (uint32_t)&end; // Clangd mark this as an error

typedef struct block {
    size_t size;
} block;

void* kmalloc_int(uint32_t sz, int align, uint32_t *phys)
{
    // This will eventually call malloc() on the kernel heap.
    // For now, though, we just assign memory at placement_address
    // and increment it by sz. Even when we've coded our kernel
    // heap, this will be useful for use before the heap is initialised.
    if (align == 1 && (placement_address & 0xFFFFF000) )
    {
        // Align the placement address;
        placement_address &= 0xFFFFF000;
        placement_address += 0x1000;
    }
    if (phys)
    {
        *phys = placement_address;
    }
    block* tmp = (block*)placement_address;
    tmp->size = sz;
    placement_address += sz;
    return tmp;
}
void kfree(void* ptr) {
    if (!ptr) return;
    placement_address -= ((block*)ptr)->size;
    memset((void*)placement_address, 0, ((block*)ptr)->size);
}
void* kmalloc_a(uint32_t sz)
{
    return kmalloc_int(sz, 1, 0);
}

void* kmalloc_p(uint32_t sz, uint32_t *phys)
{
    return kmalloc_int(sz, 0, phys);
}

void* kmalloc_ap(uint32_t sz, uint32_t *phys)
{
    return kmalloc_int(sz, 1, phys);
}

void* kmalloc(uint32_t sz)
{
    return kmalloc_int(sz, 0, 0);
}
