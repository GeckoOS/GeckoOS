/*
 * Uses a flat bitmap over 4 KB blocks. Addresses are 64-bit.
 */
#include "boot/multiboot2.h"
#include "drivers/vga.h"
#include "ports.h"
#include <mem.h>
#include <mem/physical_mem.h>
#include <stddef.h>
#include <stdint.h>
#include <terminal/printf.h>

#define BLOCK_SIZE      4096ULL
#define BLOCKS_PER_BYTE 8

static uint32_t *memory_map        = NULL;
static uint64_t max_blocks         = 0;
static uint64_t used_blocks        = 0;
static uint64_t bitmap_base        = 0; // Where the bitmap itself is stored
static uint64_t bitmap_size_blocks = 0; // Size of bitmap in blocks

// External kernel boundaries from linker script
extern uint8_t _kernel_start[];
extern uint8_t _kernel_end[];

void set_block(uint32_t bit)
{
    if (bit < max_blocks)
        memory_map[bit / 32] |= (1u << (bit % 32));
}

static void unset_block(uint32_t bit)
{
    if (bit < max_blocks)
        memory_map[bit / 32] &= ~(1u << (bit % 32));
}

int32_t find_first_free_blocks(uint32_t num_blocks)
{
    if (num_blocks == 0)
        return -1;

    for (uint64_t i = 0; i < max_blocks / 32; i++) {
        if (memory_map[i] != 0xFFFFFFFF) {
            for (int32_t j = 0; j < 32; j++) {
                if (!(memory_map[i] & (1u << j))) {
                    uint32_t free_count = 0;
                    for (uint32_t k = 0; k < num_blocks; k++) {
                        uint64_t bit = (uint64_t)i * 32 + j + k;
                        if (bit >= max_blocks)
                            break;
                        if (!(memory_map[bit / 32] & (1u << (bit % 32))))
                            free_count++;
                        else
                            break;
                    }
                    if (free_count == num_blocks)
                        return (int32_t)((uint64_t)i * 32 + j);
                }
            }
        }
    }
    return -1;
}

// Reserve a specific memory region (mark as used)
static void reserve_region(uint64_t base, uint64_t size)
{
    uint64_t start_block = base / BLOCK_SIZE;
    uint64_t num_blocks  = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    for (uint64_t i = 0; i < num_blocks && (start_block + i) < max_blocks;
         i++) {
        if (!(memory_map[(start_block + i) / 32] &
              (1u << ((start_block + i) % 32)))) {
            set_block((uint32_t)(start_block + i));
            used_blocks++;
        }
    }
}

// Add a free region (mark as unused)
static void free_region(uint64_t base, uint64_t size)
{
    uint64_t start_block = base / BLOCK_SIZE;
    uint64_t num_blocks  = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    for (uint64_t i = 0; i < num_blocks && (start_block + i) < max_blocks;i++) {
        if (memory_map[(start_block + i) / 32] &
            (1u << ((start_block + i) % 32))) {
            unset_block((uint32_t)(start_block + i));
            used_blocks--;
        }
    }
}

// Initialize from Multiboot2 memory map
extern uint64_t max_addr_used;
void initialize_memory_manager_from_mbi(uint64_t mbi_addr)
{
    // First, find the highest memory address to determine bitmap size
    uint8_t *mbi          = (uint8_t *)mbi_addr;
    uint32_t total_size   = *(uint32_t *)mbi;
    multiboot2_tag_t *tag = (multiboot2_tag_t *)(mbi + 8);
    uintptr_t mbi_end     = (uintptr_t)mbi + total_size;

    // Calculate bitmap size and find a place for it
    max_blocks = (max_addr_used + BLOCK_SIZE - 1) / BLOCK_SIZE;
    uint64_t bitmap_bytes =
        (max_blocks + BLOCKS_PER_BYTE - 1) / BLOCKS_PER_BYTE;
    uint64_t bitmap_blocks = (bitmap_bytes + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // Find a location for the bitmap (just after kernel is safe)
    bitmap_base = ((uint64_t)_kernel_end + BLOCK_SIZE - 1) & ~(BLOCK_SIZE - 1);
    memory_map  = (uint32_t *)(uintptr_t)bitmap_base;
    bitmap_size_blocks = bitmap_blocks;

    // Initialize bitmap to all used (0xFF = all blocks allocated)
    memset(memory_map, 0xFF, bitmap_bytes);
    used_blocks = max_blocks;

    // Mark available regions as free
    tag = (multiboot2_tag_t *)(mbi + 8);
    while ((uintptr_t)tag < mbi_end && tag->type != MULTIBOOT2_TAG_TYPE_END) {
        if (tag->type == MULTIBOOT2_TAG_TYPE_MMAP) {
            multiboot2_tag_mmap_t *mmap_tag = (multiboot2_tag_mmap_t *)tag;
            multiboot2_mmap_entry_t *entry  = mmap_tag->entries;
            while ((uintptr_t)entry < (uintptr_t)tag + tag->size) {
                if (entry->type == MULTIBOOT2_MEMORY_AVAILABLE) {
                    #ifdef DEBUG
                        if (entry->length) set_printf_color(VGA_COLOR_DARK_GREY);
                        else set_printf_color(VGA_COLOR_RED);
                        printf("Available memory at 0x%p with %lu bytes\n", entry->base_addr, entry->length); // %d is for int32_ts
                        if (!entry->length) {
                            printf("Available memory with negative length? (That isn't good) Halting forever...");
                            for(;;) HALT();
                        }
                    #endif
                    free_region(entry->base_addr, entry->length);
                }
                entry = (multiboot2_mmap_entry_t *)((uintptr_t)entry +
                                                    mmap_tag->entry_size);
            }
        }
        tag = (multiboot2_tag_t *)((uintptr_t)tag + ((tag->size + 7) & ~7));
    }
    #ifdef DEBUG
        set_printf_color(VGA_COLOR_WHITE);
    #endif

    // Reserve kernel regions
    reserve_region((uint64_t)_kernel_start,
                   (uint64_t)_kernel_end - (uint64_t)_kernel_start);

    // Reserve Multiboot info structure
    reserve_region(mbi_addr, total_size);

    // Reserve the bitmap itself
    reserve_region(bitmap_base, bitmap_bytes);

    // Reserve the first block (BIOS/Legacy)
    set_block(0);
    used_blocks++;

    //for lapic vectors
    #define AP_TRAMPOLINE_ADDR 0x8000
    #define AP_TRAMPOLINE_SIZE 0x1000 //4kb
    reserve_region(AP_TRAMPOLINE_ADDR, AP_TRAMPOLINE_SIZE);
}

void deinitialize_memory_region(uint64_t base_address, uint64_t size)
{ reserve_region(base_address, size); }

void *allocate_blocks(uint32_t num_blocks)
{
    if ((max_blocks - used_blocks) <= num_blocks)
        return NULL;

    int32_t start = find_first_free_blocks(num_blocks);
    if (start == -1)
        return NULL;

    for (uint32_t i = 0; i < num_blocks; i++)
        set_block((uint32_t)start + i);

    used_blocks += num_blocks;
    return (void *)(uintptr_t)((uint64_t)start * BLOCK_SIZE);
}

void free_blocks(const void *address, uint32_t num_blocks)
{
    int32_t start = (int32_t)((uintptr_t)address / BLOCK_SIZE);

    if (start < 0 || (uint64_t)start + num_blocks > max_blocks)
        return;

    for (uint32_t i = 0; i < num_blocks; i++)
        unset_block((uint32_t)start + i);

    used_blocks -= num_blocks;
}