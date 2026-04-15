#include "mem.h"
#include "drivers/vga.h"
#include <stdalign.h>

void *memcpy(void *dest, const void *src, unsigned long n) {
    // n = Number of bytes

    unsigned char *d = dest;
    const unsigned char *s = src;

    // Iterate n times, copying the byte in s into the same index in d
    for (unsigned long i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dest;
}

// [Ember2819: BEGIN - memset implementation]
void *memset(void *dest, int val, unsigned long n) {
    unsigned char *d = (unsigned char *)dest;
    for (unsigned long i = 0; i < n; i++) {
        d[i] = (unsigned char)val;
    }
    return dest;
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
extern unsigned char __bss_start;
extern unsigned char __bss_end;

static void *heap_ptr;

//no idea where it should end
static void *heap_end;

static block *free_list_head;

static unsigned long mem_max;

uint64_t kalloc_get_memory_maps_e820() {
    // they should be at 0x8000
}

void kalloc_init() {
    heap_ptr = (void *)0x200000;
    heap_end=(void*)0x500000;
    free_list_head = NULL;
}

// void* kmalloc(unsigned long size) {
//	void *ptr = heap_ptr;
//	heap_ptr+=size;
//	return ptr;
// }
static block *find_free_block(unsigned long size) {
    // if heap is empty return the heap_ptr

    block *p;
    for (p = free_list_head; p; p = p->next) {
        if (p->free && p->size >= size)
            return p;
    }
    return NULL;
}
// suposed to create the blocks if they do not exist
//
static block *create_block(unsigned long size) {

     if ((unsigned char*)heap_ptr + sizeof(block) + size > (unsigned char*)heap_end)
         return NULL;
    block *b = (block *)heap_ptr;
    b->size = size;
    b->free = 0;
    b->next = NULL;
    heap_ptr += ALIGN8(sizeof(block) + size);
    return b;
}
// tehnically we should not occupy more than needed
static void split_block(block *b, unsigned long size) {
    if (b->size <= size + sizeof(block))
        return;
    // only get what we need
    block *new_block = (block *)((char *)(b + 1) + size);
    // creating the new block
    new_block->size = b->size - size - sizeof(block);
    new_block->free = 1;
    new_block->next = b->next;
    // giving proper size to the block that we need
    b->size = size;
    b->next = new_block;
}
// allocates memory on the heap(i hope idk where the pointer above leads)
// using blocks(struct size,free,next) of memory
// i am going to trust that nobody passes size 0
void *kmalloc(unsigned long size) {

    size = ALIGN8(size);

    // trying to search for a place to allocate a block
    block *b = find_free_block(size);

    if (b) {
        b->free = 0;
        split_block(b, size);
        return (void *)(b + 1);
    }
    // if no block exists that is free increase size
    b = create_block(size);

    // if still no space do not reedem the giftcard
    if (!b) {
        return NULL;
    }
    // i have a free var in a block and
    return (void *)(b + 1);
}
// frees the block allocated at ptr by seeting the free = 1
void kfree(void *ptr) {

    if (!ptr)
        return;

    block *b = (block *)ptr - 1;

    b->free = 1;

    b->next = free_list_head;

    free_list_head = b;
}
// combinging blocks idk when i should combine them so it doesn t do that much
// lag
void combine_blocks() {
    block *b = free_list_head;

    while (b && b->next) {
        unsigned char *end = (unsigned char *)(b + 1) + b->size;

        if ((unsigned char *)b->next == end && b->next->free) {
            b->size += sizeof(block) + b->next->size;
            b->next = b->next->next;
        } else {
            b = b->next;
        }
    }
}