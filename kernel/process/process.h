#pragma once
#include "../drivers/tables/isr/isr.h"

#include <stdint.h>

#define MAX_PROCESSES 20 //!Because why not if you get a strange interrupt remember about this
//TODO: MultiCore paralel processes
//TODO: Add more states for finer debugging
typedef enum {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_TERMINATED
} process_state_t;

typedef struct process {
    uint32_t *page_directory; // CR3 value

    uint32_t esp; // kernel stack pointer
    uint32_t pid; //the process id same
    process_state_t state; //the state of the process
} process_t;



void free_process(process_t *p);
//creates a new process 
process_t *create_process(uint32_t entry_point);

//makes it actually run
void schedule(registers_t *regs);

extern void process_exit();