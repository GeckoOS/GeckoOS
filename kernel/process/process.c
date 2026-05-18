#include <process/process.h>
#include <fs/fs.h>
#include <gk/gk.h>
#include <mem.h>
#include <mem/physical_mem.h>
#include <mem/paging.h>
#include <ports.h>
#include <stdint.h>
#include <drivers/tables/isr.h>
#include <drivers/vga.h>

#define MAX_PROCESSES 16

static uint32_t current_index = 0;
uint32_t        nr_processes  = 0;
uint32_t        nextid        = 0;

typedef struct {
    Process     *data[MAX_PROCESSES];
    unsigned int head;
    unsigned int tail;
} Queue;
Queue *process_queue = NULL;

int enqueue(Queue **q, Process *v)
{
    CLI();
    if (*q == NULL) {
        *q = kmalloc(sizeof(Queue));
        if (*q == NULL) { STI(); return -1; }
        (*q)->head = 0;
        (*q)->tail = 0;
    }
    STI();

    unsigned int next = ((*q)->tail + 1) & (MAX_PROCESSES - 1);
    if (next == (*q)->head) return -1;

    (*q)->data[(*q)->tail] = v;
    (*q)->tail             = next;
    return 0;
}

Process *dequeue(Queue *q)
{
    if (q->head == q->tail) return NULL;
    Process *p = q->data[q->head];
    q->head = (q->head + 1) & (MAX_PROCESSES - 1);
    return p;
}

static page_table_t *new_address_space(void)
{
    page_table_t *pml4 = (page_table_t *)allocate_blocks(1);
    if (!pml4) return NULL;
    memset(pml4, 0, sizeof(page_table_t));
    return pml4;
}

uint32_t create_process(void *entry_point)
{
    page_table_t *pml4 = new_address_space();

    Process *proc      = kmalloc(sizeof(Process));
    proc->id           = ++nextid;
    proc->pml4         = pml4;
    proc->priority     = 1;
    proc->state        = ACTIVE;
    proc->thread_count = 1;

    Thread *main_thread       = &proc->threads[0];
    main_thread->kernel_stack = NULL;
    main_thread->parent       = proc;
    main_thread->priority     = 1;
    main_thread->state        = ACTIVE;
    main_thread->stack        = NULL;
    main_thread->stack_limit  = (void *)((uint64_t)main_thread->stack + PAGE_SIZE);
    main_thread->pgm_buf      = 0;
    main_thread->pgm_size     = 0;

    memset(&main_thread->regs, 0, sizeof(main_thread->regs));
    main_thread->regs.rip    = (uint64_t)entry_point;
    main_thread->regs.rflags = 0x200;

    /* Map a 4 KB user stack above the program buffer */
    uint64_t stack_virt = main_thread->pgm_buf + main_thread->pgm_size + PAGE_SIZE;
    void *phys = allocate_blocks(1);
    vmm_map(pml4, (uint64_t)phys, stack_virt, PTE_PRESENT | PTE_WRITABLE | PTE_USER);

    main_thread->regs.rsp = stack_virt;

    nr_processes++;
    enqueue(&process_queue, proc);
    switch_task();
    return proc->id;
}

static void execute_process(Process *proc)
{
    if (!proc->id || !proc->pml4) return;

    uint64_t entry_point = proc->threads[0].regs.rip;
    uint64_t proc_stack  = proc->threads[0].regs.rsp;

    vmm_set_pml4(proc->pml4);

    __asm__ __volatile__(
        "cli\n"
        "pushq $0x10\n"         /* SS */
        "pushq %[rsp]\n"        /* RSP */
        "pushq $0x200\n"        /* RFLAGS: IF=1 */
        "pushq $0x08\n"         /* CS  */
        "pushq %[rip]\n"        /* RIP  hehe*/
        "iretq\n"
        :
        : [rip] "r"(entry_point), [rsp] "r"(proc_stack)
        : "memory"
    );
}

static void switch_task(void)
{
    Process *v = dequeue(process_queue);
    if (v) execute_process(v);
}