// Minimal compatability header for MacOS
// Made by Slothscript with some help from AI, the code was written by hand

#ifndef GECKO_ELF_H
#define GECKO_ELF_H

#include <stdint.h>

#define EI_NIDENT 16

#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6

#define ELFMAG "\177ELF"
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define EV_CURRENT 1

#define EM_386 3

#define ET_REL 1
#define ET_EXEC 2

#define SHN_UNDEF 0
#define SHN_ABS 0xfff1

#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_RELA 4

#define STB_WEAK 2

#define ELF64_ST_BIND(x) ((x) >> 4)
#define ELF64_R_SYM(x) ((x) >> 32)
#define ELF64_R_TYPE(x) ((x) & 0xffffffff)

#define SHF_WRITE     0x1
#define SHF_ALLOC     0x2
#define SHF_EXECINSTR 0x4

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} Elf64_Shdr;

typedef struct {
    uint32_t st_name;
    unsigned char st_info;
    unsigned char st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
} Elf64_Sym;

typedef struct {
    uint64_t r_offset;
    uint64_t r_info;
} Elf64_Rel;

#endif