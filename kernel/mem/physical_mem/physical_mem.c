/*
 * Uses a flat bitmap over 4 KB blocks.  Addresses are 64-bit.
 */
#include <stdint.h>
#include <stddef.h>
#include <mem.h>
#include <mem/physical_mem.h>

#define BLOCK_SIZE      4096ULL
#define BLOCKS_PER_BYTE 8

static uint32_t *memory_map = NULL;
static uint64_t  max_blocks  = 0;
static uint64_t  used_blocks = 0;

void set_block(uint32_t bit)
{
    memory_map[bit / 32] |= (1u << (bit % 32));
}

static void unset_block(uint32_t bit)
{
    memory_map[bit / 32] &= ~(1u << (bit % 32));
}

int32_t find_first_free_blocks(uint32_t num_blocks)
{
    if (num_blocks == 0) return -1;

    for (uint64_t i = 0; i < max_blocks / 32; i++) {
        if (memory_map[i] != 0xFFFFFFFF) {
            for (int32_t j = 0; j < 32; j++) {
                if (!(memory_map[i] & (1u << j))) {
                    uint32_t free_count = 0;
                    for (uint32_t k = 0; k < num_blocks; k++) {
                        uint64_t bit = (uint64_t)i * 32 + j + k;
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

void initialize_memory_manager(uint64_t start_address, uint64_t size)
{
    memory_map  = (uint32_t *)(uintptr_t)start_address;
    max_blocks  = size / BLOCK_SIZE;
    used_blocks = max_blocks;
    memset(memory_map, 0xFF, max_blocks / BLOCKS_PER_BYTE);
}

void initialize_memory_region(uint64_t base_address, uint64_t size)
{
    int64_t align      = (int64_t)(base_address / BLOCK_SIZE);
    int64_t num_blocks = (int64_t)(size / BLOCK_SIZE);

    for (; num_blocks > 0; num_blocks--) {
        unset_block((uint32_t)align++);
        used_blocks--;
    }
    set_block(0);   /* always reserve the very first block */
}

void deinitialize_memory_region(uint64_t base_address, uint64_t size)
{
    int64_t align      = (int64_t)(base_address / BLOCK_SIZE);
    int64_t num_blocks = (int64_t)(size / BLOCK_SIZE);

    for (; num_blocks > 0; num_blocks--) {
        set_block((uint32_t)align++);
        used_blocks++;
    }
}

void *allocate_blocks(uint32_t num_blocks)
{
    if ((max_blocks - used_blocks) <= num_blocks) return NULL;

    int32_t start = find_first_free_blocks(num_blocks);
    if (start == -1) return NULL;

    for (uint32_t i = 0; i < num_blocks; i++)
        set_block((uint32_t)start + i);

    used_blocks += num_blocks;
    return (void *)(uintptr_t)((uint64_t)start * BLOCK_SIZE);
}

void free_blocks(const void *address, uint32_t num_blocks)
{
    int32_t start = (int32_t)((uintptr_t)address / BLOCK_SIZE);

    for (uint32_t i = 0; i < num_blocks; i++)
        unset_block((uint32_t)start + i);

    used_blocks -= num_blocks;
}