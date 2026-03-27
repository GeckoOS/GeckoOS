// bonk enjoyer (dorito girl)

// Should i write down everything i did? (Pumpkicks)
#ifndef _MEM_H
#define _MEM_H

#include <stdint.h>

void* memcpy(void* dest, const void* src, unsigned long n);

//Ember2819
void* memset(void* dest, int val, unsigned long n);
void* malloc(unsigned long size);

//Pumpkicks
int strlen(char* ptr);
void* kmalloc_int(uint32_t sz, int align, uint32_t *phys);
void* kmalloc_a(uint32_t sz);
void* kmalloc_p(uint32_t sz, uint32_t *phys);
void* kmalloc_ap(uint32_t sz, uint32_t *phys);
void* kmalloc(uint32_t sz);

#endif
