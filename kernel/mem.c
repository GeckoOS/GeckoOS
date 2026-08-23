#include "terminal/printf.h"
#include <mem.h>
#include <drivers/vga.h>
#include <gk/gk.h>
#include <stdbool.h>
#include <stdint.h>
#include <terminal/terminal.h>

void *memcpy(void *dest, const void *src, unsigned long n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    for (unsigned long i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

void *memmove(void *dest, const void *src, unsigned long n) { // USE rep movsb
    unsigned char *d = dest;
    const unsigned char *s = src;
    if (d < s) {
        for (unsigned long i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        for (unsigned long i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    return dest;
}

// [Ember2819: BEGIN - memset implementation]
void *memset(void *dest, int val, unsigned long n) {
    asm volatile("rep stosb" : : "D"(dest), "c"(n), "a"(val));
}
// [Ember2819: END]

// Pumpkicks
int strlen(char *ptr) {
    int i = 0;
    while (ptr[i])
        i++;
    return i;
}
 
// replace with real allocator later but should be fine for now
// kotofyt: it is not
// pumpkicks: is this enough?

static void *heap_ptr = NULL;
static void *heap_end = NULL;
static block *free_list_head = NULL;

#define BLOCK_BUFFER(x) ((uint64_t)x + sizeof(block))
#define BLOCK_BUFFER_METADATA(x) ((uint64_t)x - sizeof(block))

// Let's just have a heap with 3MB of size, that is enough (i think)
// Grub needs at least 5MB to boot so that is fine

void kalloc_init(uint64_t start, uint64_t size) {
    heap_ptr = (void*)start;
    heap_end = heap_ptr + size;

    free_list_head = (block*)heap_ptr;
    free_list_head->free = true;
    free_list_head->size = heap_end - heap_ptr;
    free_list_head->next = NULL;
}

// Divide the free_list_head into smaller blocks with the wanted size
static block *create_block(unsigned long size) {
    if (((uint64_t)free_list_head + sizeof(block) + size) > (uint64_t)heap_end) return NULL;
    if (!free_list_head) {
        printf("You should initialize the fucking heap\n");
        return NULL;
    }

    block* curl = heap_ptr;
    while (curl) {
        if (curl->free && curl->size >= size) {
            if (!curl->next) {
                free_list_head = (block*)((BLOCK_BUFFER(curl)) + size);
                free_list_head->free = true;
                free_list_head->size = ((uint64_t)heap_end - (uint64_t)free_list_head) - sizeof(block);
                free_list_head->next = NULL;

                curl->next = free_list_head;
            }

            curl->free = false;
            curl->size = size;

            return curl;
        }
        // If the next block is free and the size of this block plus the next are the more than the wanted size, mix them and return it
        if (curl->free && curl->next) {
            if (curl->next->free) {
                if ((curl->size + curl->next->size) >= size) {
                    curl->next = curl->next->next;
                    if (!curl->next) {
                        curl->next = (block*)(BLOCK_BUFFER(curl) + size);
                        free_list_head = curl->next;

                        curl->next->free = true;
                        curl->size = ((uint64_t)heap_end - (uint64_t)free_list_head) - sizeof(block);
                        curl->next->next = NULL;
                    }

                    curl->size = size;
                    curl->free = false;

                    return curl;
                }
            }
        }
        curl = curl->next;
    }

    return NULL;
}
// tehnically we should not occupy more than needed
// allocates memory on the heap(i hope idk where the pointer above leads)
// using blocks(struct size,free,next) of memory
// i am going to trust that nobody passes size 0
void *kmalloc(unsigned long size) {
    size = ALIGN8(size);

    // if no block exists that is free increase size
    block* b = create_block(size);
    // i have a free var in a block and
    return b ? (void *)(BLOCK_BUFFER(b)) : NULL;
}
void *kmalloc_4m(unsigned long size) {
    size = ALIGN4M(size);
    // if no block exists that is free increase size
    block* b = create_block(size);

    // if still no space do not reedem the giftcard
    if (!b) {
        return NULL;
    }
    // i have a free var in a block and
    return (void *)(ALIGN4M((uint64_t)b)) + 1;
}

void dump_heap() {
    block* curl = heap_ptr;
    while (curl) {
        #ifdef DEBUG
            printf("  Block at %p (Buffer at %p) with size = %d bytes, free = %d, next = %x\n", curl, ((uint64_t)curl + sizeof(block)), curl->size, curl->free, curl->next);
        #else
            printf("  Block at %p with size = %d bytes, free = %d, next = %p\n", curl, curl->size, curl->free, curl->next);
        #endif

        curl = curl->next;
    }
    printf("Free memory: %dMb\n", free_list_head->size / 1048576);
}

// frees the block allocated at ptr by seeting the free = 1
void kfree(void *ptr) {
    if (!ptr) return;

    block *b = (block*)(BLOCK_BUFFER_METADATA(ptr));

    b->free = true;
}