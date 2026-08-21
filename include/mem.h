// bonk enjoyer (dorito girl)

// Should i write down everything i did? (Pumpkicks)
#ifndef _MEM_H
#define _MEM_H
//idk but they say aligning is important
#include <stddef.h>
#include <stdint.h>
#define ALIGN8(x) (((x) + 7) & ~7)

#define KERNEL_VIRT_BASE 0xFFFFFFFF80000000

//this is block of memory
typedef struct block block ;
struct block{
    int size;
    int free;
    block* next;
    
};

void* memcpy(void* dest, const void* src, unsigned long n);
void* memmove(void* dest, const void* src, unsigned long n);

static block* find_free_block(unsigned long size) ;
//Ember2819
void* memset(void* dest, int val, unsigned long n);
//helper functions to allocate memory
static block* find_free_block(unsigned long size) ;
static block* create_block(unsigned long size);
static void split_block(block * b,unsigned long size);

void kalloc_init();
void* kmalloc(unsigned long size);
void* kmalloc_aligned(unsigned long size, uint32_t alignment);

void kfree(void* p);
void combine_blocks();
//Pumpkicks
int strlen(char* ptr);

// these are defined in <stddef.h>
/*
typedef unsigned int size_t;
typedef int ssize_t;
*/

typedef struct Buffer {
    unsigned char* bytes;
    size_t size;
} Buffer_t;

#define assert(x) if(!x){printf("%s: assertion failed (%s)\n", __func__, #x); for(;;);}

#endif
