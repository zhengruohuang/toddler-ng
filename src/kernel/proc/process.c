/*
 * Process manager
 */


#include "common/include/inttypes.h"
#include "common/include/errno.h"
// #include "common/include/memory.h"
// #include "kernel/include/hal.h"
#include "kernel/include/mem.h"
#include "kernel/include/lib.h"
// #include "kernel/include/coreimg.h"
// #include "kernel/include/exec.h"
#include "kernel/include/proc.h"


static int proc_salloc_id;

static struct process_list processes;
struct process *kernel_proc;


static ulong alloc_proc_id(struct process *p)
{
    ulong id = (ulong)p;
    return id;
}


struct process *create_process(ulong parent_id, char *name, enum process_type type)
{
    int mon_err = EOK;
    struct process *p = NULL;

    // Allocate a process struct
    p = (struct process *)salloc(proc_salloc_id);
    assert(p);

    // Assign a proc id
    p->proc_id = alloc_proc_id(p);
    p->parent_id = parent_id;

    // Setup the process
    p->name = strdup(name);
    p->url = NULL;

    p->type = type;
    p->state = PROCESS_STATE_ENTER;
    p->user_mode = type != PROCESS_TYPE_KERNEL;
    p->priority = 0;

    // Thread list
    //create_thread_lists(p);

    // Page table
    if (type == PROCESS_TYPE_KERNEL) {
        //p->page_dir_pfn = hal->kernel_page_dir_pfn;
    } else {
        //p->page_dir_pfn = palloc(hal->user_page_dir_page_count);
    }
    assert(p->page_dir_pfn);

    // Init the address space
    if (type != PROCESS_TYPE_KERNEL) {
        //hal->init_addr_space(p->page_dir_pfn);
    }

    // Init dynamic area
    //create_dalloc(p);

    // Memory layout
    p->memory.entry_point = 0;

    p->memory.program_start = 0;
    p->memory.program_end = 0;

    p->memory.heap_start = 0;
    p->memory.heap_end = 0;

    if (type == PROCESS_TYPE_KERNEL) {
        p->memory.dynamic_top = 0;
        p->memory.dynamic_bottom = 0;
    } else {
        p->memory.dynamic_top = p->dynamic.cur_top;
        p->memory.dynamic_bottom = p->dynamic.cur_top;
    }

    // Msg handlers
    //hashtable_create(&p->msg_handlers, 0, NULL, NULL);

    // ASID
    if (type == PROCESS_TYPE_KERNEL) {
        p->asid = 0;
    } else {
        //p->asid = asid_alloc();
    }

    // Insert the process into process list
//     p->prev = NULL;
//     p->next = processes.next;
//     processes.next = p;
//     processes.count++;

    // Done
    return p;
}

int load_image(struct process *p, char *url)
{
    // Load image
    //ulong img = (ulong)get_core_file_addr_by_name(url); // FIXME: should use namespace service
    //ulong entry = 0, vaddr_start = 0, vaddr_end = 0;
    //int succeed = hal->load_exe(img, p->page_dir_pfn, &entry, &vaddr_start, &vaddr_end);
    //int succeed = load_exec(img, p->page_dir_pfn, &entry, &vaddr_start, &vaddr_end);

    //if (!succeed) {
    //    return 0;
    //}

    // Calculae rounded heap start
    //ulong heap_start = vaddr_end;
    //if (heap_start % PAGE_SIZE) {
    //    heap_start /= PAGE_SIZE;
    //    heap_start++;
    //    heap_start *= PAGE_SIZE;
    //}

    // Map the initial page for heap
    //ulong heap_paddr = PFN_TO_ADDR(palloc(1));
    //succeed = hal->map_user(
    //    p->page_dir_pfn,
    //    heap_start, heap_paddr, PAGE_SIZE,
    //    0, 1, 1, 0
    //);
    //assert(succeed);

    // Set memory layout
    //p->memory.entry_point = entry;
    //p->memory.program_start = vaddr_start;
    //p->memory.program_end = vaddr_end;
    //p->memory.heap_start = heap_start;
    //p->memory.heap_end = heap_start;

    return 1;
}


void init_process()
{
    kprintf("Initializing process manager\n");

    // Create salloc obj
    proc_salloc_id = salloc_create(sizeof(struct process), 0, 0, NULL, NULL);

    // Init process list
    //processes.count = 0;
    //processes.next = NULL;

    // Create the kernel process
    //kernel_proc = create_process(-1, "kernel", "coreimg://tdlrkrnl.bin", PROCESS_TYPE_KERNEL, 0);

    kprintf("\tSalloc ID: %d, Kernel process ID: %x\n", proc_salloc_id, kernel_proc->proc_id);
}
