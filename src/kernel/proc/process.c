/*
 * Process manager
 */


#include "common/include/inttypes.h"
#include "common/include/errno.h"
#include "common/include/abi.h"
#include "common/include/elf.h"
#include "kernel/include/mem.h"
#include "kernel/include/lib.h"
#include "kernel/include/struct.h"
#include "kernel/include/kernel.h"
#include "kernel/include/proc.h"
#include "kernel/include/syscall.h"


/*
 * Kernel and system process
 */
static struct process *kernel_proc = NULL;
static ulong kernel_pid = 0;

struct process *get_kernel_proc()
{
    return kernel_proc;
}

ulong get_kernel_pid()
{
    return kernel_pid;
}


/*
 * Process list
 */
static list_t processes;

struct process *acquire_process(ulong id)
{
    struct process *p = NULL;

    list_foreach_exclusive(&processes, n) {
        struct process *cur = list_entry(n, struct process, node);
        if (cur->pid == id) {
            p = cur;
            ref_count_inc(&p->ref_count);
            break;
        }
    }

    return p;
}

void release_process(struct process *p)
{
    panic_if(!p, "Inconsistent acquire/release pair\n");
    panic_if(p->ref_count.value <= 0, "Inconsistent ref_count\n");

    ref_count_dec(&p->ref_count);
}

int process_exists(ulong pid)
{
    int exists = 0;

    list_foreach_exclusive(&processes, n) {
        struct process *cur = list_entry(n, struct process, node);
        if (cur->pid == pid) {
            exists = 1;
            break;
        }
    }

    return exists;
}

ulong get_num_processes()
{
    ulong num = 0;

    list_access_exclusive(&processes) {
        num = processes.count;
    }

    return num;
}

void process_stats(ulong *count, struct proc_stat *buf, size_t buf_size)
{
    list_access_exclusive(&processes) {
        if (count) {
            *count = processes.count;
        }

        ulong i = 0;
        list_foreach(&processes, n) {
            if (i >= buf_size) {
                break;
            }

            struct process *p = list_entry(n, struct process, node);
            strcpy(buf[i].name, p->name);
            buf[i].num_inuse_vm_blocks = ref_count_read(&p->vm.num_active_blocks);
            buf[i].num_pages = ref_count_read(&p->vm.num_palloc_pages);

            i++;
        }
    }
}


/*
 * Init
 */
static salloc_obj_t proc_salloc_obj;

void init_process()
{
    kprintf("Initializing process manager\n");

    // Create salloc obj
    salloc_create_default(&proc_salloc_obj, "process", sizeof(struct process));

    // Init process list
    list_init(&processes);

    // Create the kernel process
    kernel_pid = create_process(-1, "kernel", PROCESS_TYPE_KERNEL);
    kernel_proc = acquire_process(kernel_pid);
    kprintf("\tKernel process ID: %x\n", kernel_pid);
}


/*
 * Cleanup
 */
static inline int _stop_all_threads(struct process *p)
{
    int all_threads_stopped = 0;

    int state = -1;
    spinlock_exclusive_int(&p->lock) {
        state = p->state;
    }

    if (state == PROCESS_STATE_STOPPED) {
        ulong thread_count = list_count_exclusive(&p->threads);

        if (thread_count) {
            purge_wait_queue(p);
            purge_ipc_queue(p);
        } else {
            all_threads_stopped = 1;
        }
    }

    return all_threads_stopped;
}

static inline int _free_all_vm_blocks(struct process *p)
{
    int all_vm_blocks_freed = 0;

    int state = -1;
    spinlock_exclusive_int(&p->lock) {
        state = p->state;
    }

    if (state == PROCESS_STATE_ZOMBIE) {
        if (ref_count_is_zero(&p->vm.num_active_blocks)) {
            all_vm_blocks_freed = 1;
        } else {
            vm_purge(p);
        }
    }

    return all_vm_blocks_freed;
}

static void process_cleaner(ulong param)
{
    struct process *p = (void *)param;
    //kprintf("Cleaner for process @ %p\n", p);

    int all_threads_stopped = 0;
    int all_vm_blocks_freed = 0;

    while (!all_threads_stopped || !all_vm_blocks_freed) {
        //kprintf("Wait for event\n");

        // Wait for next cleanup event
        event_wait(&p->cleanup_event);

        // Free up unused VM blocks
        vm_move_sanit_to_avail(p);

        // Stop all threads
        if (!all_threads_stopped && p->state == PROCESS_STATE_STOPPED) {
            all_threads_stopped = _stop_all_threads(p);
            if (all_threads_stopped) {
                //kprintf("All threads stopped for process %p\n", p);
                ipc_notify_system(SYS_NOTIF_PROCESS_STOPPED, p->pid);
            }
        }

        // Free up all VM blocks
        if (!all_vm_blocks_freed && p->state == PROCESS_STATE_ZOMBIE) {
            all_vm_blocks_freed = _free_all_vm_blocks(p);
            //if (all_vm_blocks_freed) {
            //    kprintf("All VM blocks stopped for process %p\n", p);
            //}
        }
    }

    // Free VM block structs
    vm_destory(p);

    // Free page table
    get_hal_exports()->free_addr_space(p->page_table);

    // Free ASID
    free_asid(p->asid);

    // Free process data structure
    list_remove_exclusive(&processes, &p->node);
    free(p->name);
    sfree(p);

    // Done
    kprintf("Clean for process %p done\n", p);
    syscall_thread_exit_self(0);
}


/*
 * Process creation
 */
static volatile ulong cur_asid_seq = 1;

static inline ulong alloc_asid_seq()
{
    ulong asid = atomic_fetch_and_add(&cur_asid_seq, 1);
    return asid;
}

static ulong alloc_process_id(struct process *p)
{
    ulong id = (ulong)p;
    return id;
}

ulong create_process(ulong parent_id, char *name, enum process_type type)
{
    struct process *p = NULL;

    // Allocate a process struct
    p = (struct process *)salloc(&proc_salloc_obj);
    panic_if(!p, "Unable to allocate process struct\n");

    // Assign a proc id
    ulong pid = alloc_process_id(p);
    p->pid = pid;
    p->parent_pid = parent_id;

    // ASID
    p->asid = type == PROCESS_TYPE_KERNEL ? 0 : alloc_asid();

    // Setup the process
    p->name = strdup(name);
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
    spinlock_init(&p->page_table_lock);
    panic_if(!p->page_table, "Unable to allocate page table");

    // Memory layout
    p->vm.entry_point = 0;

    p->vm.program.start = 0;
    p->vm.program.end = 0;

    p->vm.dynamic.start = hal_get_vaddr_base();
    p->vm.dynamic.end = hal_get_vaddr_limit();

    ref_count_init(&p->vm.num_active_blocks, 0);
    list_init(&p->vm.avail_unmapped);
    list_init(&p->vm.inuse_mapped);
    list_init(&p->vm.sanit_unmapped);
    list_init(&p->vm.sanit_mapped);
    list_init(&p->vm.reuse_mapped);

    vm_create(p);

    // Thread list
    list_init(&p->threads);

    // Msg handlers
    //list_init(&p->serial_msgs);
    p->popup_msg_handler = 0;

    // Cleaner
    event_init(&p->cleanup_event, 0);

    // Init lock
    spinlock_init(&p->lock);
    ref_count_init(&p->ref_count, 1);

    // Insert the process into process list
    list_push_back_exclusive(&processes, &p->node);

    // Create cleaner
    if (type != PROCESS_TYPE_KERNEL) {
        ulong param = (ulong)p;
        create_and_run_kernel_thread(tid, t, &process_cleaner, param, NULL);
    }

    // Done
    return pid;
}


/*
 * Process exit
 */
int exit_process(struct process *proc, ulong status)
{
    spinlock_exclusive_int(&proc->lock) {
        proc->state = PROCESS_STATE_STOPPED;
    }

    event_signal(&proc->cleanup_event);
    return 0;
}

int recycle_process(struct process *proc)
{
    spinlock_exclusive_int(&proc->lock) {
        proc->state = PROCESS_STATE_ZOMBIE;
    }

    event_signal(&proc->cleanup_event);
    return 0;
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

int load_coreimg_elf(struct process *p, void *img)
{
    ulong vrange_start = -1ul;
    ulong vrange_end = 0;

    elf_native_program_t *prog_header = NULL;
    elf_native_header_t *elf_header = (elf_native_header_t *)img;
    kprintf("Load ELF image @ %p, page table @ %p\n", img, p->page_table);

    // For every segment, map and load
    for (int i = 0; i < elf_header->elf_phnum; i++) {
        //kprintf("\tSegment #%d\n", i);

        // Get program header
        if (i) {
            prog_header = (void *)((ulong)prog_header + (ulong)elf_header->elf_phentsize);
        } else {
            prog_header = (void *)(img + elf_header->elf_phoff);
        }

        // Map the segment to destination's vaddr space
        if (prog_header->program_type == 1 && prog_header->program_memsz) {
            // Get range
            ulong vaddr_start =
                    align_down_vaddr((ulong)prog_header->program_vaddr, PAGE_SIZE);
            ulong vaddr_end =
                    align_up_vaddr((ulong)prog_header->program_vaddr + prog_header->program_memsz, PAGE_SIZE);
            ulong vaddr_range = vaddr_end - vaddr_start;
            ulong vpages = get_vpage_count(vaddr_range);

            if (vaddr_start < vrange_start) vrange_start = vaddr_start;
            if (vaddr_end > vrange_end) vrange_end = vaddr_end;

            // Map
            ppfn_t ppfn = palloc_direct_mapped(vpages);
            paddr_t paddr = ppfn_to_paddr(ppfn);

            int mapped = 0;
            spinlock_exclusive_int(&p->page_table_lock) {
                mapped = get_hal_exports()->map_range(p->page_table,
                                                      vaddr_start, paddr, vaddr_range,
                                                      1, 1, 1, 0, 1);
            }
            panic_if(mapped != vpages, "Map failed");

            //kprintf("\t\tMapping range @ %p - %p to %llx, num pages: %ld\n",
            //        vaddr_start, vaddr_end, (u64)paddr, vpages);

            // Zero memory
            void *paddr_win = hal_cast_paddr_to_kernel_ptr(paddr);
            memzero(paddr_win, vaddr_range);

            // Copy data if there's any
            if (prog_header->program_filesz) {
                void *paddr_win_copy = paddr_win + ((ulong)prog_header->program_vaddr - vaddr_start);
                void *vaddr_copy = (void *)((ulong)img + (ulong)prog_header->program_offset);
                //kprintf("\t\tCopying from %p -> %p, size: %lx\n", vaddr_copy, paddr_win_copy, prog_header->program_filesz);
                memcpy(paddr_win_copy, vaddr_copy, prog_header->program_filesz);
            }
        }
    }

    // Set up address space info
    p->vm.entry_point = elf_header->elf_entry;
    p->vm.program.start = vrange_start;
    p->vm.program.end = vrange_end;

    //kprintf("\tEntry @ %p\n", p->vm.entry_point);

    struct vm_block *b = vm_alloc(p, vrange_start, vrange_end - vrange_start, 0);
    panic_if(!b, "Unable to allocate VM block for program code!\n");

    return EOK;
}
