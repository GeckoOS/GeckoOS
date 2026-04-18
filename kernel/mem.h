#ifndef MEM_H
#define MEM_H

#include "stddef.h"
#include "stdint.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define HEAP_SIZ 1024

// y'know what, i surrender living off below 1MB. a cool kernel can't just only use 25KB of memory (b)
extern uint8_t end;
#define EXT_MEM_BASE (uint32_t)&end 
#define EXT_MEM_SIZ  ((size_t)1 << 20) // 1MB o' size

extern uint8_t __bss_start__;
extern uint8_t __bss_end__;

#define BSS_END   (&__bss_end__)
#define BSS_START (&__bss_start__)

#define ALLOC_EXTRALOG 0
#define PRINT_ALLOCS   0

struct bios_da {
  uint16_t com_port[4];
  uint16_t lpt_port[3];
  uint16_t ebda_addr;
  uint16_t flags;
  uint8_t  pcjr;
  uint16_t usable_memory;
} __attribute__((packed));

void parse_bda();

void zero_bss();

void kmalloc_init();

#if !ALLOC_EXTRALOG
void *malloc(size_t size);
#else
#define malloc(size) ({ void *malloc_log(size_t siz); void *ptr = malloc_log((size)); printkf("malloc %d at line %d file %s\n", (size), __LINE__, __FILE__); ptr; })
#endif

void free(void *ptr);

void  *malloc_align(size_t siz, size_t align);
void   free_align(void *ptr);

size_t getused();
size_t getmaxused();
size_t gettrueused();

#endif