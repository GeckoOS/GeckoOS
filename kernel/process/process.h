#pragma once
#include "../drivers/tables/isr/isr.h"
#include "mem/virtual_mem/paging.h"
#include <stdint.h>

typedef enum {
    INVALID,
    SLEEPING,
    ACTIVE,
} Proc_State;

typedef struct Process Process;
typedef struct Thread Thread;

typedef Process process_t;
typedef Thread thread_t;

//Struct for multitasking not implemented
typedef struct Thread {
    Process *parent;
    void *stack;
    void *stack_limit;
    void *kernel_stack;
    uint32_t priority;
    Proc_State state;
    uint32_t /*  */pgm_buf; // Base address of loaded program (entry point is separate)
    uint32_t pgm_size; // Size of loaded program
    registers_t regs;
} Thread;
//The process struct stores a process ready for multitasking(using more cpu cores)
typedef struct Process {
    uint32_t id;
    uint32_t priority;
    page_directory_t *page_dir;
    Proc_State state;
    Thread threads[5]; // TODO: Change for runtime multitasking/using dynamic
                       // memory
    uint32_t thread_count;
} Process;
//A linear scheduler
static void switch_task();
//Creates and runs process for now it accepts a function pointer 
uint32_t create_process(void* entry_point);
//Runs the created process should be called by the scheduler
static void execute_process(Process* proc);