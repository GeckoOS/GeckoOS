#pragma once

#include <stdint.h>
#include "../../../terminal/terminal.h"
//the asm functions that does the magic without triple fault
extern void paging_enable(uint32_t);
//physmem is the memeory that we are going to split into pages
void paging_init(uint32_t physmem_kb);