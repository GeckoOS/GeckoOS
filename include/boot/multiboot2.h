#pragma once
#include <stdint.h>


/** Written into the Multiboot2 header embedded in the kernel image. */
#define MULTIBOOT2_HEADER_MAGIC     UINT32_C(0xE85250D6)

/** Architecture field value for 32-bit protected mode (i386). */
#define MULTIBOOT2_ARCH_I386        UINT32_C(0)

#define MULTIBOOT2_BOOTLOADER_MAGIC UINT32_C(0x36D76289)

#define MULTIBOOT2_HEADER_TAG_END               0
#define MULTIBOOT2_HEADER_TAG_INFORMATION_REQUEST 1
#define MULTIBOOT2_HEADER_TAG_ADDRESS           2
#define MULTIBOOT2_HEADER_TAG_ENTRY_ADDRESS     3
#define MULTIBOOT2_HEADER_TAG_CONSOLE_FLAGS     4
#define MULTIBOOT2_HEADER_TAG_FRAMEBUFFER       5
#define MULTIBOOT2_HEADER_TAG_MODULE_ALIGN      6

#define MULTIBOOT2_TAG_TYPE_END             0
#define MULTIBOOT2_TAG_TYPE_CMDLINE         1
#define MULTIBOOT2_TAG_TYPE_BOOT_LOADER_NAME 2
#define MULTIBOOT2_TAG_TYPE_MODULE          3
#define MULTIBOOT2_TAG_TYPE_BASIC_MEMINFO   4
#define MULTIBOOT2_TAG_TYPE_BOOTDEV         5
#define MULTIBOOT2_TAG_TYPE_MMAP            6
#define MULTIBOOT2_TAG_TYPE_VBE             7
#define MULTIBOOT2_TAG_TYPE_FRAMEBUFFER     8
#define MULTIBOOT2_TAG_TYPE_ELF_SECTIONS    9
#define MULTIBOOT2_TAG_TYPE_APM             10
#define MULTIBOOT2_TAG_TYPE_EFI32           11
#define MULTIBOOT2_TAG_TYPE_EFI64           12
#define MULTIBOOT2_TAG_TYPE_SMBIOS          13
#define MULTIBOOT2_TAG_TYPE_ACPI_OLD        14  /* RSDP v1 */
#define MULTIBOOT2_TAG_TYPE_ACPI_NEW        15  /* RSDP v2 */

/* ── Memory map entry types ──────────────────────────────────────────────── */

#define MULTIBOOT2_MEMORY_AVAILABLE         1
#define MULTIBOOT2_MEMORY_RESERVED          2
#define MULTIBOOT2_MEMORY_ACPI_RECLAIMABLE  3
#define MULTIBOOT2_MEMORY_NVS               4
#define MULTIBOOT2_MEMORY_BADRAM            5

typedef struct multiboot2_info_header {
    uint32_t total_size;
    uint32_t reserved;
} __attribute__((packed)) multiboot2_info_header_t;

/** Every tag in the MBI begins with this common prefix. */
typedef struct multiboot2_tag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed)) multiboot2_tag_t;

/** MULTIBOOT2_TAG_TYPE_BASIC_MEMINFO */
typedef struct multiboot2_tag_basic_meminfo {
    uint32_t type;
    uint32_t size;
    uint32_t mem_lower;   /**< Kilobytes of lower memory (below 1 MB). */
    uint32_t mem_upper;   /**< Kilobytes of upper memory (above 1 MB). */
} __attribute__((packed)) multiboot2_tag_basic_meminfo_t;

/** One entry in the MULTIBOOT2_TAG_TYPE_MMAP tag. */
typedef struct multiboot2_mmap_entry {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;        /**< One of MULTIBOOT2_MEMORY_* */
    uint32_t reserved;
} __attribute__((packed)) multiboot2_mmap_entry_t;

/** MULTIBOOT2_TAG_TYPE_MMAP */
typedef struct multiboot2_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    /* Followed by (size - 16) / entry_size entries of type multiboot2_mmap_entry_t */
} __attribute__((packed)) multiboot2_tag_mmap_t;

/** MULTIBOOT2_TAG_TYPE_CMDLINE */
typedef struct multiboot2_tag_cmdline {
    uint32_t type;
    uint32_t size;
    char     string[];    /**< Null-terminated command line string. */
} __attribute__((packed)) multiboot2_tag_cmdline_t;

/** MULTIBOOT2_TAG_TYPE_BOOT_LOADER_NAME */
typedef struct multiboot2_tag_bootloader_name {
    uint32_t type;
    uint32_t size;
    char     string[];
} __attribute__((packed)) multiboot2_tag_bootloader_name_t;

/** MULTIBOOT2_TAG_TYPE_FRAMEBUFFER */
typedef struct multiboot2_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
    uint16_t reserved;
} __attribute__((packed)) multiboot2_tag_framebuffer_t;

/** MULTIBOOT2_TAG_TYPE_ACPI_OLD / MULTIBOOT2_TAG_TYPE_ACPI_NEW */
typedef struct multiboot2_tag_acpi {
    uint32_t type;
    uint32_t size;
    uint8_t  rsdp[];      /**< Raw RSDP bytes (8 bytes for v1, 36 for v2). */
} __attribute__((packed)) multiboot2_tag_acpi_t;

/**
 * Advance a tag pointer to the next tag.
 * Multiboot2 tags are 8-byte aligned relative to the MBI base.
 */
#define MULTIBOOT2_TAG_NEXT(tag) \
    ((multiboot2_tag_t *)(((uintptr_t)(tag) + (tag)->size + 7) & ~UINT32_C(7)))
