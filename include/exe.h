#pragma once

#include "mem.h"
#include <elf.h>
#include <stddef.h>

int elf_load_stage1(Elf64_Ehdr* hdr);
int elf_load_stage2(Elf64_Ehdr* hdr);

void dumpelf(unsigned char* elf);
void runelf(Buffer_t elf);