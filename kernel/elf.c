#include "mem.h"
#include "process/process.h"
#include <stdbool.h>
#include <stddef.h>
#include <elf.h>
#include <commands.h>
#include <stdint.h>
#include <terminal/printf.h>

bool is_elf_valid(Elf64_Ehdr* hdr) {
    if (!hdr) return false;
    if (*(uint32_t*)&hdr->e_ident[0] != *(uint32_t*)ELFMAG) return false;
    return true;
}

bool is_elf_supported(Elf64_Ehdr *hdr) {
	if (!is_elf_valid(hdr)) {
		printf("Invalid ELF File.\n");
		return false;
	}
	if (hdr->e_ident[EI_CLASS] != ELFCLASS64) {
		printf("Unsupported ELF File Class.\n");
		return false;
	}
	if (hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		printf("Unsupported ELF File byte order.\n");
		return false;
	}
	if (hdr->e_machine != EM_X86_64) {
		printf("Unsupported ELF File target.\n");
		return false;
	}
	if (hdr->e_ident[EI_VERSION] != EV_CURRENT) {
		printf("Unsupported ELF File version.\n");
		return false;
	}
	if (hdr->e_type != ET_REL && hdr->e_type != ET_EXEC) {
		printf("Unsupported ELF File type.\n");
		return false;
	}

	return true;
}

// Helpers
static Elf64_Shdr* elf_sheader(Elf64_Ehdr* hdr) { return (Elf64_Shdr*)((uint64_t)hdr + hdr->e_shoff); }
static Elf64_Shdr* elf_section(Elf64_Ehdr* hdr, int idx) { return &elf_sheader(hdr)[idx]; }

static char* elf_str_table(Elf64_Ehdr* hdr) {
	if (hdr->e_shstrndx == SHN_UNDEF) return NULL;
	return (char*)hdr + elf_section(hdr, hdr->e_shstrndx)->sh_offset;
}

static char* elf_lookup_string(Elf64_Ehdr* hdr, int offset) {
	char* strtab = elf_str_table(hdr);
	if (strtab == NULL) return NULL;
	return strtab + offset;
}

static Elf64_Sym* elf_lookup_symbol() {

}

static uint64_t elf_get_symval(Elf64_Ehdr* hdr, int table, uint32_t idx) {
	if(table == SHN_UNDEF || idx == SHN_UNDEF) return 0;
	Elf64_Shdr* symtab = elf_section(hdr, table);

	uint32_t symtab_entries = symtab->sh_size / symtab->sh_entsize;
	if (idx >= symtab_entries) {
		printf("Symbol Index out of Range (%d:%u).\n", table, idx);
		return -1;
	}

	int symaddr = (int)hdr + symtab->sh_offset;
	Elf64_Sym* symbol = &((Elf64_Sym*)symaddr)[idx];

    if(symbol->st_shndx == SHN_UNDEF) {
        // External symbol, lookup value
        Elf64_Shdr* strtab = elf_section(hdr, symtab->sh_link);
        const char* name = (const char *)hdr + strtab->sh_offset + symbol->st_name;

        void* target = elf_lookup_symbol(name);

        if (target == NULL) {
            // Extern symbol not found
            if (ELF64_ST_BIND(symbol->st_info) & STB_WEAK) {
                // Weak symbol initialized as 0
                return 0;
            } else {
                printf("Undefined External Symbol : %s.\n", name);
                return -1;
            }
        } else {
            return (uint64_t)target;
        }
    } else if(symbol->st_shndx == SHN_ABS) {
		// Absolute symbol
		return symbol->st_value;
	} else {
		// Internally defined symbol
		Elf64_Shdr* target = elf_section(hdr, symbol->st_shndx);
		return (uint64_t)hdr + symbol->st_value + target->sh_offset;
	}

    return -1;
}

void elf_load_stage1(Elf64_Ehdr* hdr) {
	Elf64_Shdr* shdr = elf_sheader(hdr);

	for (uint32_t i = 0; i < hdr->e_shnum; i++) {
		Elf64_Shdr* section = &shdr[i];

		// printf ("%d\n", section->sh_type);
		if (section->sh_type == SHT_NOBITS) {
			if (!section->sh_size) continue;

			if (section->sh_flags & SHF_ALLOC) {
				void* m = kmalloc(section->sh_size);
				memset(m, 0, section->sh_size);

				section->sh_offset = (uint64_t)m - (uint64_t)hdr;
				printf("Allocated memory for a section (%ld).\n", section->sh_size);
			}
		}
	}
}

int elf_do_reloc(Elf64_Ehdr* hdr, Elf64_Rel* rel, Elf64_Shdr* reltab);
int elf_load_stage2(Elf64_Ehdr* hdr) {
	Elf64_Shdr* shdr = elf_sheader(hdr);

	for (uint32_t i = 0; i < hdr->e_shnum; i++) {
		Elf64_Shdr* section = &shdr[i];

		if (section->sh_type == SHT_REL) {
			for (uint32_t idx = 0; idx < section->sh_size / section->sh_entsize; idx++) {
				Elf64_Rel* rel = &((Elf64_Rel*)((uint64_t)hdr + section->sh_offset))[idx];

				if (elf_do_reloc(hdr, rel, section) == -1) {
					printf("Failed to relocate symbol.\n");
					return -1;
				}
			}
		}
	}
	return 0;
}

# define DO_386_32(S, A)	((S) + (A))
# define DO_386_PC32(S, A, P)	((S) + (A) - (P))

int elf_do_reloc(Elf64_Ehdr* hdr, Elf64_Rel* rel, Elf64_Shdr* reltab) {
	Elf64_Shdr* target = elf_section(hdr, reltab->sh_info);

	uint64_t addr = (uint64_t)hdr + target->sh_offset;
	int* ref = (int*)(addr + rel->r_offset);

	int symval = 0;
	if (ELF64_R_SYM(symval) != SHN_UNDEF) {
		symval = elf_get_symval(hdr, reltab->sh_link, ELF64_R_SYM(rel->r_info));
		if (symval == -1) return -1;
	}

	switch (ELF64_R_TYPE(rel->r_info)) {
		case R_386_NONE:
			// No relocation
			break;
		case R_386_32:
			// Symbol + Offset
			*ref = DO_386_32(symval, *ref);
			break;
		case R_386_PC32:
			// Symbol + Offset - Section Offset
			*ref = DO_386_PC32(symval, *ref, (uint64_t)ref);
			break;
		default:
			// Relocation type not supported, display error and return
			printf("Unsupported Relocation Type (%d).\n", ELF32_R_TYPE(rel->r_info));
			return -1;
	}
	return symval;
}

static inline void* elf_load_rel(Elf64_Ehdr *hdr) {
	elf_load_stage1(hdr);
	if(elf_load_stage2(hdr) == -1) {
		printf("Unable to load ELF file.\n");
		return NULL;
	}
	// TODO : Parse the program header (if present)
	return (void*)hdr->e_entry;
}

void* elf_load_file(void* file) {
	Elf64_Ehdr *hdr = (Elf64_Ehdr*)file;
	if(!is_elf_supported(hdr)) {
		printf("ELF File cannot be loaded.\n");
		return NULL;
	}
	switch(hdr->e_type) {
		case ET_EXEC:
			elf_load_stage1(hdr);
			elf_load_stage2(hdr);
			Elf64_Shdr* shdr = elf_sheader(hdr);

			uint64_t data;
			for (uint32_t i = 0; i < hdr->e_shnum; i++) {
				Elf64_Shdr section = shdr[i];

				printf("%s %d\n", elf_lookup_string(hdr, section.sh_name), section.sh_offset);
				for (int i = 0; i < section.sh_size; i++) {
					printf("%X ", *(uint8_t*)(((uint64_t)hdr + section.sh_offset) + i));
				} printf("\n");
				if (elf_lookup_string(hdr, section.sh_name)[1] == 'd') data = (uint64_t)hdr + section.sh_offset;
			}
			for (uint32_t i = 0; i < hdr->e_shnum; i++) {
				Elf64_Shdr section = shdr[i];

				printf("%s %d\n", elf_lookup_string(hdr, section.sh_name), section.sh_offset);
				for (int i = 0; i < section.sh_size; i++) {
					printf("%X ", *(uint8_t*)(((uint64_t)hdr + section.sh_offset) + i));
				} printf("\n");
				if (elf_lookup_string(hdr, section.sh_name)[1] == 't') {
					create_process(hdr + section.sh_offset, (uint64_t)hdr + section.sh_offset, (void*)data, data);
				}
			}
			return NULL;
		case ET_REL:
			return elf_load_rel(hdr);
	}
	return NULL;
}