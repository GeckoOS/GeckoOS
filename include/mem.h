// bonk enjoyer (dorito girl)

// Should i write down everything i did? (Pumpkicks)
#ifndef _MEM_H
#define _MEM_H
//idk but they say aligning is important
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ALIGN8(x) (((x) + 7) & ~7)
#define ALIGN4M(x) (((x) + 4095) & ~4095)

#define KERNEL_VIRT_BASE 0xFFFFFFFF80000000

//this is block of memory

// #define BLOCK_MAGIC 0xDDDEAAD

typedef struct block block;
struct block{
    // uint32_t magic; // Who need magic numbers
    size_t size;
    bool free;
    block* next;
};

void* memcpy(void* dest, const void* src, unsigned long n);
void* memmove(void* dest, const void* src, unsigned long n);

//Ember2819
void* memset(void* dest, int val, unsigned long n);
//helper functions to allocate memory
static block* create_block(unsigned long size);

void kalloc_init(uint64_t start, uint64_t size);
void* kmalloc(unsigned long size);
void* kmalloc_4m(unsigned long size);

void dump_heap();

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
