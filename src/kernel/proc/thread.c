/*
 * Thread manager
 */


#include "common/include/inttypes.h"
#include "common/include/stdarg.h"
#include "kernel/include/lib.h"
#include "kernel/include/mem.h"
#include "kernel/include/struct.h"
#include "kernel/include/kernel.h"
#include "kernel/include/proc.h"
#include "kernel/include/syscall.h"


/*
 * Kernel demo thread
 */
static void kernel_demo_thread(ulong param)
{
    //while (1);

    do {
        int index = param;
        int cpu_id = hal_get_cur_mp_seq();
        //kprintf("Kernel demo thread #%d on CPU #%d%s!\n",
        //        index, cpu_id , index == cpu_id ? ", TID == CPU ID" : "");
        syscall_thread_yield();
    } while (1);
}


/*
 * Thread list
 */
static list_t threads;

struct thread *acquire_thread(ulong id)
{
    struct thread *t = NULL;

    list_foreach_exclusive(&threads, n) {
        struct thread *cur = list_entry(n, struct thread, node);
        if (cur->tid == id) {
            t = cur;
            ref_count_inc(&t->ref_count);
            break;
        }
    }

    //kprintf("acquired thread @ %p\n", t);
    return t;
}

void release_thread(struct thread *t)
{
    panic_if(!t, "Inconsistent acquire/release pair\n");
    panic_if(t->ref_count.value <= 0, "Inconsistent ref_count: %ld, tid: %lx\n",
             t->ref_count.value, t->tid);

    ref_count_dec(&t->ref_count);

    //kprintf("release thread @ %p, int enabled: %d\n", t, t->lock.int_enabled);
}

int thread_exists(ulong tid)
{
    int exists = 0;

    list_foreach_exclusive(&threads, n) {
        struct thread *cur = list_entry(n, struct thread, node);
        if (cur->tid == tid) {
            exists = 1;
            break;
        }
    }

    return exists;
}


/*
 * Init
 */
static salloc_obj_t thread_salloc_obj;

void init_thread()
{
    kprintf("Initializing thread manager\n");

    // Create salloc obj
    salloc_create(&thread_salloc_obj, sizeof(struct thread), 0, 0, NULL, NULL);

    // Init thread list
    list_init(&threads);

//     // Create kernel demo threads
//     for (int i = 0; i < 4; i++) {
//         create_krun(tid, t, &kernel_demo_thread, i, -1) {
//             kprintf("\tKernel demo thread created, thread ID: %p, thread block base: %p\n", tid, t->memory.block_base);
//         }
//     }
}


/*
 * Thread creation
 */
#define KERNEL_TLS_SIZE     (PAGE_SIZE)
#define KERNEL_STACK_SIZE   (PAGE_SIZE)

#define USER_TLS_SIZE       (PAGE_SIZE * 4)
#define USER_STACK_SIZE     (PAGE_SIZE * 4)

static ulong alloc_thread_id(struct thread *t)
{
    ulong id = (ulong)t;
    return id;
}

static struct thread *create_exec_context(struct process *p,
                                          struct thread_attri *attri)
{
    // Allocate a thread struct
    struct thread *t = (struct thread *)salloc(&thread_salloc_obj);
    assert(t);

    // Assign a thread id
    t->tid = alloc_thread_id(t);

    // Setup the thread
    t->pid = p->pid;
    t->user_mode = p->user_mode;
    t->page_table = p->page_table;
    t->asid = p->asid;
    t->state = THREAD_STATE_ENTER;

    ulong seq = atomic_fetch_and_add(&p->thread_create_count.value, 1);
    t->is_main = !seq;

    // Round up stack size and tls size
    ulong default_stack_size = t->user_mode ? USER_STACK_SIZE : KERNEL_STACK_SIZE;
    ulong default_tls_size = t->user_mode ? USER_TLS_SIZE : KERNEL_TLS_SIZE;
    ulong stack_size = attri && attri->stack_size ? ALIGN_UP(attri->stack_size, PAGE_SIZE) : default_stack_size;
    ulong tls_size = attri && attri->tls_size ? ALIGN_UP(attri->tls_size, PAGE_SIZE) : default_tls_size;

    // Setup sizes
    t->memory.tls.size = tls_size;
    t->memory.stack.size = stack_size;
    t->memory.block_size = t->memory.tls.size + t->memory.stack.size;

    // Setup offsets
    t->memory.tls.start_offset = 0;
    t->memory.stack.limit_offset = t->memory.tls.start_offset + t->memory.tls.size;
    t->memory.stack.top_offset = t->memory.stack.limit_offset + t->memory.stack.size;

    // Allocate memory
    if (p->type == PROCESS_TYPE_KERNEL) {
        t->memory.block_base = (ulong)palloc_ptr(t->memory.block_size / PAGE_SIZE);
        //kprintf("Kernel thread block allocated paddr @ %p\n", (void *)t->memory.block_base);

        t->memory.tls.start_paddr_ptr = (void *)(t->memory.block_base + t->memory.tls.start_offset);
        t->memory.stack.top_paddr_ptr = (void *)(t->memory.block_base + t->memory.stack.top_offset);
    } else {
        // Allocate a dynamic block
        t->memory.block_base = vm_alloc(p, t->memory.block_size, PAGE_SIZE, VM_ATTRI_DATA);

        // Allocate physical memory
        t->memory.tls.start_paddr_ptr = palloc_ptr(1);
        void *initial_stack_paddr_ptr = palloc_ptr(1);
        t->memory.stack.top_paddr_ptr = initial_stack_paddr_ptr + PAGE_SIZE;

        // Map virtual to physical
        get_hal_exports()->map_range(p->page_table,
                                     t->memory.block_base + t->memory.tls.start_offset,
                                     cast_ptr_to_paddr(t->memory.tls.start_paddr_ptr),
                                     t->memory.tls.size,
                                     1, 1, 1, 0, 0);
        get_hal_exports()->map_range(p->page_table,
                                     t->memory.block_base + t->memory.stack.top_offset - PAGE_SIZE,
                                     cast_ptr_to_paddr(initial_stack_paddr_ptr),
                                     PAGE_SIZE,
                                     1, 1, 1, 0, 0);
    }

    // Adjust stack top to make some room for msg block
    const ulong msg_block_size = align_up_ulong(sizeof(msg_t), 16);
    t->memory.msg.size = msg_block_size;
    t->memory.stack.top_offset -= msg_block_size;
    t->memory.stack.top_paddr_ptr -= msg_block_size;
    t->memory.msg.start_offset = t->memory.stack.top_offset;
    t->memory.msg.start_paddr_ptr = t->memory.stack.top_paddr_ptr;

    // Add some extra room to separate stack and msg proxy
    // Not something necessary, just to be extra safe
    t->memory.stack.top_offset -= 16;
    t->memory.stack.top_paddr_ptr -= 16;

    // Init TIB
    struct thread_info_block *tib = (struct thread_info_block *)t->memory.tls.start_paddr_ptr;
    memzero(tib, sizeof(struct thread_info_block));

    tib->self = (struct thread_info_block *)(t->memory.block_base + t->memory.tls.start_offset);
    tib->pid = t->pid;
    tib->tid = t->tid;

    tib->msg = (void *)(t->memory.block_base + t->memory.msg.start_offset);
    tib->tls = (void *)(t->memory.block_base + t->memory.tls.start_offset + sizeof(struct thread_info_block));
    tib->tls_size = t->memory.tls.size - sizeof(struct thread_info_block);

    // Init lock
    spinlock_init(&t->lock);
    ref_count_init(&t->ref_count, 1);
    ref_count_inc(&p->ref_count);

    // Insert the thread into global and the process local thread lists
    list_push_back_exclusive(&threads, &t->node);
    list_push_back_exclusive(&p->threads, &t->node_proc);

    // Done
    return t;
}

ulong create_thread(struct process *p, ulong entry_point, ulong param,
                    struct thread_attri *attri)
{
    // Create thread execution context
    struct thread *t = create_exec_context(p, attri);

    // Context
    get_hal_exports()->init_context(
        &t->context, entry_point, param,
        t->memory.block_base + t->memory.stack.top_offset,
        t->user_mode);

    // Done
    return t->tid;
}


/*
 * Exit thread
 */
void exit_thread(struct thread *t)
{
    kprintf("Exiting thread @ %p\n", t);

    access_process(t->pid, proc) {
        // TODO: check thread state

        // Clean up dynamic area
        if (proc->type == PROCESS_TYPE_KERNEL) {
            pfree_ptr((void *)t->memory.block_base);
        } else {
        }

        access_wait_queue_exclusive(wait_queue) {
            list_remove_exclusive(&threads, &t->node);
            list_remove_exclusive(&proc->threads, &t->node_proc);

            wake_on_object(proc, t, WAIT_ON_THREAD, t->tid, 0, 0);
            if (t->is_main) {
                wake_on_object(proc, t, WAIT_ON_MAIN_THREAD, t->tid, 0, 0);
            }
        }

        ref_count_dec(&t->ref_count);
        panic_if(!ref_count_is_zero(&t->ref_count),
                 "Inconsistent ref count: %ld, tid @ %lx\n",
                 t->ref_count.value, t->tid);

        sfree(t);
        ref_count_dec(&proc->ref_count);
    }
}

// static void destroy_thread(struct process *p, struct thread *t)
// {
//     //kprintf("[Thread] To destory absent thread, process: %s\n", p->name);
//
//     spin_lock_int(&t->lock);
//
//     // Scheduling
//     clean_sched(t->sched);
//
//     // FIXME: Temporarily disable this part to avoid some weird issues
// #if 1
//     // Dynamic area
//     if (p->type == process_kernel) {
//         pfree(ADDR_TO_PFN(t->memory.block_base));
//     } else {
//         ulong vaddr = 0;
//         ulong paddr = 0;
//
//         assert(t->memory.block_base != 0xeffbe000);
//
//         // TLB shootdown first
//         trigger_tlb_shootdown(p->asid, t->memory.block_base, t->memory.block_size);
//
//         // Msg send
//         vaddr = t->memory.block_base + t->memory.msg_send_offset;
//         paddr = t->memory.msg_send_paddr;
//         //kprintf("To unmap msg send, vaddr @ %p, paddr @ %p\n", (void *)vaddr, (void *)paddr);
//         hal->unmap_user(p->page_dir_pfn, vaddr, paddr, t->memory.msg_send_size);
//         pfree(ADDR_TO_PFN(paddr));
//
//         // Msg recv
//         vaddr = t->memory.block_base + t->memory.msg_recv_offset;
//         paddr = t->memory.msg_recv_paddr;
//         //kprintf("To unmap msg recv, vaddr @ %p, paddr @ %p\n", (void *)vaddr, (void *)paddr);
//         hal->unmap_user(p->page_dir_pfn, vaddr, paddr, t->memory.msg_recv_size);
//         pfree(ADDR_TO_PFN(paddr));
//
//         // TLS
//         vaddr = t->memory.block_base + t->memory.tls_start_offset;
//         paddr = t->memory.tls_start_paddr;
//         vaddr = ALIGN_DOWN(vaddr, PAGE_SIZE);
//         paddr = ALIGN_DOWN(paddr, PAGE_SIZE);
//         //kprintf("To unmap tls, vaddr @ %p, paddr @ %p\n", (void *)vaddr, (void *)paddr);
//         hal->unmap_user(p->page_dir_pfn, vaddr, paddr, t->memory.tls_size);
//         pfree(ADDR_TO_PFN(paddr));
//
//         // Stack
//         vaddr = t->memory.block_base + t->memory.stack_limit_offset;
//         paddr = hal->get_paddr(p->page_dir_pfn, vaddr);
//         //kprintf("To unmap stack, vaddr @ %p, paddr @ %p\n", (void *)vaddr, (void *)paddr);
//         hal->unmap_user(p->page_dir_pfn, vaddr, paddr, t->memory.stack_size);
//         pfree(ADDR_TO_PFN(paddr));
//
//         // Free dynamic area
//         dfree(p, t->memory.block_base);
//     }
// #endif
//
//     spin_unlock_int(&t->lock);
//
//     sfree(t);
// }
//
// void destroy_absent_threads(struct process *p)
// {
//     struct thread *t = pop_front(&p->threads.absent);
//     while (t) {
//         destroy_thread(p, t);
//         t = pop_front(&p->threads.absent);
//     }
// }


/*
 * Save context
 */
void thread_save_context(struct thread *t, struct reg_context *ctxt)
{
    memcpy(&t->context, ctxt, sizeof(struct reg_context));
}


/*
 * Change thread state
 */
const char *get_thread_state_name(int state)
{
    switch (state) {
    case THREAD_STATE_NORMAL:
        return "normal";
    case THREAD_STATE_WAIT:
        return "wait";
    case THREAD_STATE_EXIT:
        return "exit";
    default:
        return "unknown";
    }

    return "unknown";
}

void set_thread_state(struct thread *t, int state)
{
    spinlock_exclusive_int(&t->lock) {
        // Check if transition is valid
        switch (state) {
        case THREAD_STATE_NORMAL:
            panic_if(t->state != THREAD_STATE_ENTER && t->state != THREAD_STATE_WAIT,
                     "Invalid thread state transition: %s -> %s\n",
                     get_thread_state_name(t->state), get_thread_state_name(state));
            break;
        case THREAD_STATE_WAIT:
            panic_if(t->state != THREAD_STATE_NORMAL,
                     "Invalid thread state transition: %s -> %s\n",
                     get_thread_state_name(t->state), get_thread_state_name(state));
            break;
        case THREAD_STATE_EXIT:
            panic_if(t->state != THREAD_STATE_NORMAL && t->state != THREAD_STATE_WAIT,
                     "Invalid thread state transition: %s -> %s\n",
                     get_thread_state_name(t->state), get_thread_state_name(state));
            break;
        default:
            panic("Invalid thread state transition: %s -> %s\n",
                  get_thread_state_name(t->state), get_thread_state_name(state));
            break;
        }

        t->state = state;
    }
}

void run_thread(struct thread *t)
{
    ref_count_inc(&t->ref_count);

    set_thread_state(t, THREAD_STATE_NORMAL);
    sched_put(t);
}
