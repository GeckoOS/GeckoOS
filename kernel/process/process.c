#include "process.h"
#include "../drivers/tables/paging/page_allocator.h"


process_t* process_list[MAX_PROCESSES]; //the process array so the kernel can manage themS
uint32_t process_count = 0; //how many processes you have hope it s less than max

//idk if i should explain something here
process_t *current_process; 
uint32_t avilable_pid = 0; //process_id
uint32_t current_index = 0;


static inline uint32_t read_esp(void) {
    uint32_t esp;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    return esp;
}
void setup_process_stack(process_t *p, uint32_t entry) {
    uint32_t *stack = (uint32_t *)alloc_page() + 1024;

    //magic here (actually just how ret instruction in asm works and the flags code segment and all that)
    *(--stack) = (uint32_t)process_exit; // return address
    // CPU will iret into this
    *(--stack) = 0x202; // EFLAGS
    *(--stack) = 0x08;  // CS
    *(--stack) = entry; // EIP

    // pusha registers
    for (int i = 0; i < 8; i++)
        *(--stack) = 0;

    p->esp = (uint32_t)stack;
}
process_t *create_process(uint32_t entry_point) {

    process_t *p = (process_t *)alloc_page(); 

    p->pid = avilable_pid++;
    //p->eip = entry_point;

    //allocate page directory
    p->page_directory = (uint32_t *)alloc_page();

    //clear page directory
    for (int i = 0; i < 1024; i++)
        p->page_directory[i] = 0;

    // map kernel space 
    for (int i = 768; i < 1024; i++) {
        extern uint32_t page_directory[];
        p->page_directory[i] = page_directory[i];
    }

    // user stack
    void *stack = alloc_page();

    // map stack at high user address
    map_page(p->page_directory, 0xBFFFF000, (uint32_t)stack, 3);

    setup_process_stack(p, entry_point);
    process_list[process_count++] = p;
    return p;
}
process_t *next_process(void) {
    for (int i = 0; i < process_count; i++) {
        // the current_index of the current active process
        current_index = (current_index + 1) % process_count;
        // shouldn t run dead processes but idk
        if (process_list[current_index]->state == PROCESS_READY)
            return process_list[current_index];
    }
    // if there are no processes i think you got bigger problems but idk
    return current_process;
}
void schedule(registers_t *regs) {
    // Save current process state
   if (current_process) {
        //push the regs
        current_process->esp = (uint32_t)regs;

        if (current_process->state == PROCESS_TERMINATED) {
            free_process(current_process);
        }
    }
    // Pick next process
    process_t *next = next_process();
    current_process = next;

    //switch address space
    asm volatile("mov %0, %%cr3" ::"r"(next->page_directory));

    //switch stack for real
    asm volatile("mov %0, %%esp" ::"r"(next->esp));
}
// Terminates the process (it was not a question)
void process_exit() {
    current_process->state = PROCESS_TERMINATED;

    // Force scheduler to run
    asm volatile("int $32"); // trigger timer interrupt manually
}
// does what it says cleans up after a process remember you have limited memory
void free_process(process_t *p) {
    // free stack
    free_page((void *)(p->esp & ~0xFFF));

    // free page directory
    free_page(p->page_directory);

    for (int i = 0; i < process_count; i++) {
        if (process_list[i] == p) {
            for (int j = i; j < process_count - 1; j++)
                process_list[j] = process_list[j + 1];

            process_count--;
            break;
        }
    }

    free_page(p);
}