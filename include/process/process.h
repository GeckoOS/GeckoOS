#pragma once
#include <drivers/tables/isr.h>
#include <mem/paging.h>
#include <stdint.h>

typedef enum {
    INVALID,
    SLEEPING,
    ACTIVE,
} Proc_State;

typedef struct Process Process;
typedef struct Thread  Thread;

typedef Process process_t;
typedef Thread  thread_t;

typedef struct Thread {
    Process    *parent;
    void       *stack;
    void       *stack_limit;
    void       *kernel_stack;
    uint32_t    priority;
    Proc_State  state;
    uint64_t    pgm_buf;
    uint64_t    pgm_size;
    registers_t regs;
} Thread;

typedef struct Process {
    uint32_t      id;
    uint32_t      priority;
    page_table_t *pml4;
    Proc_State    state;
    Thread        threads[5];
    uint32_t      thread_count;
} Process;

static void    switch_task(void);
uint32_t       create_process(void *entry_point);
static void    execute_process(Process *proc);