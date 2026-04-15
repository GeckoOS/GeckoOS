#include "process.h"
#include "fs/fs.h"
#include "gk/gk.h"
#include "mem/mem.h"
#include "mem/physical_mem/physical_mem.h"
#include "mem/virtual_mem/paging.h"
#include "ports.h"
#include "stdint.h"
#include "tables/isr/isr.h"
#include "vga.h"

#define MAX_PROCESSES 16

static uint32_t current_index = 0;
uint32_t nr_processes         = 0;
uint32_t nextid               = 0;

// going to use a queue
//  for scheduling processes
//Queue in c yeye
typedef struct {
    Process *data[MAX_PROCESSES];
    unsigned int head;
    unsigned int tail;
} Queue;
Queue *process_queue = NULL;

int enqueue(Queue **q, Process *v)
{
    CLI();

    if (*q == NULL) {
        *q = kmalloc(sizeof(Queue));

        if (*q == NULL) {
            STI();
            return -1;
        }

        (*q)->head = 0;
        (*q)->tail = 0;
    }

    STI();

    unsigned int next = ((*q)->tail + 1) & (MAX_PROCESSES - 1);

    if (next == (*q)->head)
        return -1;

    (*q)->data[(*q)->tail] = v;
    (*q)->tail             = next;

    return 0;
}
Process *dequeue(Queue *q)
{
    if (q->head == q->tail)
        return NULL;

    Process *p = q->data[q->head];
    q->head    = (q->head + 1) & (MAX_PROCESSES - 1);
    return p;
}

page_directory_t *new_address_space(void)
{
    page_directory_t *rtn = kmalloc(4096);
    // print_int((uint32_t)rtn);
    if (!rtn)
        return NULL;

    memset(rtn, 0, sizeof *rtn);
    return rtn;
}
// Creates a process and a thread for it and returns the pid of it
uint32_t create_process(void *entry_point)
{
    page_directory_t *address_space = new_address_space();

    Process *proc      = kmalloc(sizeof(Process));
    proc->id           = ++nextid;
    proc->page_dir     = address_space;
    proc->priority     = 1;
    proc->state        = ACTIVE;
    proc->thread_count = 1;

    // Create thread
    Thread *main_thread       = &proc->threads[0];
    main_thread->kernel_stack = NULL;
    main_thread->parent       = proc;
    main_thread->priority     = 1;
    main_thread->state        = ACTIVE;
    main_thread->stack        = NULL;
    main_thread->stack_limit =
        (void *)((uint32_t)main_thread->stack + PAGE_SIZE);

    // Load program into memory; The open file FD addresses and Program Buffer
    //   addresses are virtual and mapped to valid physical memory from
    //   malloc(), syscall_open(), etc.
    uint8_t *pgm_buf      = NULL;
    uint32_t pgm_size     = 0;
    main_thread->pgm_buf  = (uint32_t)pgm_buf;
    main_thread->pgm_size = pgm_size;

    memset(&main_thread->regs, 0, sizeof main_thread->regs);
    main_thread->regs.eip    = (int32_t)entry_point;
    main_thread->regs.eflags = 0x200;

    // Create & Map userspace stack
    void *stack     = (void *)((uint8_t *)main_thread->pgm_buf +
                           main_thread->pgm_size + PAGE_SIZE);
    void *phys_addr = allocate_blocks(1); // Only 4KB for now
    map_address(address_space, (uint32_t)phys_addr, (uint32_t)stack,
                PTE_PRESENT | PTE_READ_AND_WRITE);
    // Create & map userspace memory for arguments (heap?), above stack
    void *args = (void *)((uint8_t *)stack + PAGE_SIZE);
    phys_addr  = allocate_blocks(1);
    map_address(address_space, (uint32_t)phys_addr, (uint32_t)args,
                PTE_PRESENT | PTE_READ_AND_WRITE);

    main_thread->regs.esp = (uint32_t)main_thread->stack;
    main_thread->regs.ebp = main_thread->regs.esp;

    nr_processes++;

    if (enqueue(&process_queue, proc) != 0) {
        
    }
   

	//TODO:add a real scheduler  this just takes each process and executes it
    switch_task();

    return proc->id;

}

void execute_process(Process *proc)
{
    // Get running process
    ;
    
    if (!proc->id || !proc->page_dir) {

        return;
    }

    // Get EIP/ESP of main thread
    int32_t entry_point = proc->threads[0].regs.eip;
    uint32_t proc_stack = proc->threads[0].regs.esp;

    // Switch to process address space
    set_page_directory(proc->page_dir);

    int32_t stack = 0;

    __asm__ __volatile__("mov %%esp, %0" : "=a"(stack));

    // Execute process in kernel mode
    __asm__ __volatile__("cli\n"
                         "pushl $return_label\n"
                         "pushl $0x200\n"   // EFLAGS
                         "pushl $0x08\n"    // CS (kernel code segment)
                         "pushl %[entry]\n" // EIP

                         "iretl\n"
                         "return_label:"
                         :
                         : [entry] "r"(entry_point)
                         : "memory");
	
}
// Very Fair process scheduler FIFO if a process blocks the cpu IDGAF
void switch_task()
{
    Process *v;
    v = dequeue(process_queue);
    
    if (v != 0) {
       
        execute_process(v);
    } else {

        return;
    }
}