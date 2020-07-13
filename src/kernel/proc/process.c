/*
 * Process manager
 */


#include "common/include/inttypes.h"
#include "common/include/errno.h"
#include "common/include/abi.h"
#include "common/include/elf.h"
// #include "common/include/memory.h"
// #include "kernel/include/hal.h"
#include "kernel/include/mem.h"
#include "kernel/include/lib.h"
#include "kernel/include/struct.h"
// #include "kernel/include/coreimg.h"
// #include "kernel/include/exec.h"
#include "kernel/include/kernel.h"
#include "kernel/include/proc.h"


/*
 * Kernel and system process
 */
static ulong kernel_pid = 0;

ulong get_kernel_pid()
{
    return kernel_pid;
}


/*
 * Process list
 */
static slist_t processes;

struct process *acquire_process(ulong id)
{
    struct process *p = NULL;

    slist_foreach_exclusive(&processes, n) {
        struct process *cur = n->node;
        if (cur->pid == id) {
            p = cur;
            spinlock_lock_int(&p->lock);
            atomic_inc(&p->ref_count.value);
            spinlock_unlock_int(&p->lock);
            break;
        }
    }

    if (p) {
        spinlock_lock_int(&p->lock);
    }

    //kprintf("acquired process @ %s\n", p->name);
    return p;
}

void release_process(struct process *p)
{
    //kprintf("released process: %s, int enabled: %d\n", p->name, p->lock.int_enabled);

    panic_if(!p, "Inconsistent acquire/release pair\n");
    panic_if(!p->lock.locked, "Inconsistent acquire/release pair\n");
    panic_if(p->ref_count.value <= 0, "Inconsistent ref_count\n");

    atomic_dec(&p->ref_count.value);
    spinlock_unlock_int(&p->lock);
}


/*
 * Init
 */
static int proc_salloc_id;

void init_process()
{
    kprintf("Initializing process manager\n");

    // Create salloc obj
    proc_salloc_id = salloc_create(sizeof(struct process), 0, 0, NULL, NULL);

    // Init process list
    slist_create(&processes);
    kprintf("\tSalloc ID: %d\n", proc_salloc_id);

    // Create the kernel process
    kernel_pid = create_process(-1, "kernel", PROCESS_TYPE_KERNEL);
    kprintf("\tKernel process ID: %x\n", kernel_pid);
}


/*
 * Process creation
 */
static ulong alloc_process_id(struct process *p)
{
    ulong id = (ulong)p;
    return id;
}

ulong create_process(ulong parent_id, char *name, enum process_type type)
{
    struct process *p = NULL;

    // Allocate a process struct
    p = (struct process *)salloc(proc_salloc_id);
    panic_if(!p, "Unable to allocate process struct\n");

    // Assign a proc id
    p->pid = alloc_process_id(p);
    p->parent_pid = parent_id;

    // ASID
    p->asid = type == PROCESS_TYPE_KERNEL ? 0 : 0;

    // Setup the process
    p->name = strdup(name);
    p->url = NULL;

    p->type = type;
    p->user_mode = type != PROCESS_TYPE_KERNEL;
    p->state = PROCESS_STATE_ENTER;
    p->priority = 0;

    // Page table and init user addr space
    if (type == PROCESS_TYPE_KERNEL) {
        p->page_table = get_hal_exports()->kernel_page_table;
    } else {
        p->page_table = get_hal_exports()->init_addr_space();
    }
    panic_if(!p->page_table, "Unable to allocate page table");

    // Memory layout
    p->memory.entry_point = 0;

    p->memory.program_start = 0;
    p->memory.program_end = 0;

    p->memory.heap_start = 0;
    p->memory.heap_end = 0;

    p->memory.dynamic_top = 0;
    p->memory.dynamic_bottom = 0;

    // Dynamic area
    //create_dalloc(p);

    // Threads
    slist_create(&p->threads);

    // Msg handlers
    //hashtable_create(&p->msg_handlers, 0, NULL, NULL);

    // Init lock
    spinlock_init(&p->lock);
    p->ref_count.value = 1;

    // Insert the process into process list
    slist_push_back_exclusive(&processes, p);

    // Done
    return p->pid;
}


/*
 * Load image from coreimg
 */
#if ARCH_WIDTH == 32
typedef struct elf32_header     elf_native_header_t;
typedef struct elf32_program    elf_native_program_t;
typedef struct elf32_section    elf_native_section_t;
typedef Elf32_Addr              elf_native_addr_t;
#else
typedef struct elf64_header     elf_native_header_t;
typedef struct elf64_program    elf_native_program_t;
typedef struct elf64_section    elf_native_section_t;
typedef Elf64_Addr              elf_native_addr_t;
#endif

int load_coreimg_elf(struct process *p, char *url, void *img)
{
    panic_if(!spinlock_is_locked(&p->lock), "process must be locked!\n");
    p->url = strdup(url);

    ulong vrange_start = -1ul;
    ulong vrange_end = 0;

    elf_native_program_t *prog_header = NULL;
    elf_native_header_t *elf_header = (elf_native_header_t *)img;
    kprintf("\tLoad ELF image @ %p, page table @ %p\n", img, p->page_table);

    // For every segment, map and load
    for (int i = 0; i < elf_header->elf_phnum; i++) {
        kprintf("\t\tSegment #%d\n", i);

        // Get program header
        if (i) {
            prog_header = (void *)((ulong)prog_header + (ulong)elf_header->elf_phentsize);
        } else {
            prog_header = (void *)(img + elf_header->elf_phoff);
        }

        // Map the segment to destination's vaddr space
        if (prog_header->program_memsz) {
            // Get range
            ulong vaddr_start =
                    align_down_vaddr((ulong)prog_header->program_vaddr, PAGE_SIZE);
            ulong vaddr_end =
                    align_up_vaddr((ulong)prog_header->program_vaddr + prog_header->program_memsz, PAGE_SIZE);
            ulong vaddr_range = vaddr_end - vaddr_start;
            ulong vpages = get_vpage_count(vaddr_range);

            if (vaddr_start < vrange_start) vrange_start = vaddr_start;
            if (vaddr_end > vrange_end) vrange_end = vaddr_end;

//             for (ulong vaddr2 = 0; vaddr2 < 0x80000000ul; vaddr2 += PAGE_SIZE) {
//                 paddr_t paddr2 = get_hal_exports()->translate(p->page_table, vaddr2);
//                 kprintf("Old map: %x\n", paddr2);
//             }

            //paddr_t paddr2 = get_hal_exports()->translate(p->page_table, vaddr_start);
            //kprintf("Old map: %x\n", paddr2);

            // Map
            ppfn_t ppfn = palloc_direct_mapped(vpages);
            paddr_t paddr = ppfn_to_paddr(ppfn);
            int mapped = get_hal_exports()->map_range(p->page_table, vaddr_start, paddr, vaddr_range, 1, 1, 1, 0, 1);
            panic_if(mapped != vpages, "Map failed");

            kprintf("\t\t\tMapping range @ %p - %p to %llx, num pages: %ld\n",
                    vaddr_start, vaddr_end, (u64)paddr, vpages);

            //paddr_t paddr3 = get_hal_exports()->translate(p->page_table, vaddr_start);
            //kprintf("New map: %x\n", paddr3);

            // Zero memory
            void *paddr_win = cast_paddr_to_ptr(paddr);
            memzero(paddr_win, vaddr_range);

            // Copy data if there's any
            if (prog_header->program_filesz) {
                void *paddr_win_copy = paddr_win + ((ulong)prog_header->program_vaddr - vaddr_start);
                void *vaddr_copy = (void *)((ulong)img + (ulong)prog_header->program_offset);
                kprintf("\t\t\tCopying from %p -> %p, size: %lx\n", vaddr_copy, paddr_win_copy, prog_header->program_filesz);
                memcpy(paddr_win_copy, vaddr_copy, prog_header->program_filesz);
            }
        }
    }

    // Set up address space info
    p->memory.entry_point = elf_header->elf_entry;
    p->memory.program_start = vrange_start;
    p->memory.program_end = vrange_end;

    kprintf("\t\tEntry @ %p\n", p->memory.entry_point);

    // Set up initial heap
    ulong heap_vaddr = align_up_vaddr(vrange_end + PAGE_SIZE, PAGE_SIZE);
    p->memory.heap_start = p->memory.heap_end = heap_vaddr;

    return EOK;
}
