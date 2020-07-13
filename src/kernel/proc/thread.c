/*
 * Thread manager
 */


#include "common/include/inttypes.h"
//#include "common/include/memory.h"
//#include "kernel/include/hal.h"
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
        syscall_yield();
    } while (1);
}


/*
 * Thread list
 */
static slist_t threads;

struct thread *acquire_thread(ulong id)
{
    struct thread *t = NULL;

    slist_foreach_exclusive(&threads, n) {
        struct thread *cur = n->node;
        if (cur->tid == id) {
            t = cur;
            spinlock_lock_int(&t->lock);
            atomic_inc(&t->ref_count.value);
            spinlock_unlock_int(&t->lock);
            break;
        }
    }

    if (t) {
        spinlock_lock_int(&t->lock);
    }

    //kprintf("acquired thread @ %p\n", t);
    return t;
}

void release_thread(struct thread *t)
{
    //kprintf("release thread @ %p, int enabled: %d\n", t, t->lock.int_enabled);

    panic_if(!t, "Inconsistent acquire/release pair\n");
    panic_if(!t->lock.locked, "Inconsistent acquire/release pair\n");
    panic_if(t->ref_count.value <= 0, "Inconsistent ref_count\n");

    atomic_dec(&t->ref_count.value);
    spinlock_unlock_int(&t->lock);
}


/*
 * Init
 */
static int thread_salloc_id;

void init_thread()
{
    kprintf("Initializing thread manager\n");

    // Create salloc obj
    thread_salloc_id = salloc_create(sizeof(struct thread), 0, 0, NULL, NULL);
    kprintf("\tThread salloc ID: %d\n", thread_salloc_id);

    // Init thread list
    slist_create(&threads);

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
static ulong alloc_thread_id(struct thread *t)
{
    ulong id = (ulong)t;
    return id;
}

ulong create_thread(struct process *p, ulong entry_point, ulong param,
                             int pin_cpu_id, ulong stack_size, ulong tls_size)
{
    panic_if(!spinlock_is_locked(&p->lock), "process must be locked!\n");

    // Allocate a thread struct
    struct thread *t = (struct thread *)salloc(thread_salloc_id);
    assert(t);

    // Assign a thread id
    t->tid = alloc_thread_id(t);

    // Setup the thread
    t->pid = p->pid;
    t->user_mode = p->user_mode;
    t->page_table = p->page_table;
    t->asid = p->asid;
    t->state = THREAD_STATE_ENTER;

    // Round up stack size and tls size
    stack_size = ALIGN_UP(stack_size ? stack_size : PAGE_SIZE, PAGE_SIZE);
    tls_size = ALIGN_UP(tls_size ? tls_size : PAGE_SIZE, PAGE_SIZE);

    // Setup sizes
    t->memory.msg_send_size = PAGE_SIZE;
    t->memory.msg_recv_size = PAGE_SIZE;
    t->memory.tls_size = tls_size;
    t->memory.stack_size = stack_size;
    t->memory.block_size = t->memory.msg_send_size + t->memory.msg_recv_size +
                            t->memory.tls_size + t->memory.stack_size;

    // Setup offsets
    t->memory.msg_send_offset = 0;
    t->memory.msg_recv_offset = t->memory.msg_send_offset + t->memory.msg_send_size;
    t->memory.tls_start_offset = t->memory.msg_recv_offset + t->memory.msg_recv_size;
    t->memory.stack_limit_offset = t->memory.tls_start_offset + t->memory.tls_size;
    t->memory.stack_top_offset = t->memory.stack_limit_offset + stack_size;

    // Allocate memory
    if (p->type == PROCESS_TYPE_KERNEL) {
        t->memory.block_base = (ulong)palloc_ptr(t->memory.block_size / PAGE_SIZE);
        //kprintf("Kernel thread block allocated paddr @ %p\n", (void *)t->memory.block_base);

        t->memory.msg_send_paddr = t->memory.block_base + t->memory.msg_send_offset;
        t->memory.msg_recv_paddr = t->memory.block_base + t->memory.msg_recv_offset;
        t->memory.tls_start_paddr = t->memory.block_base + t->memory.tls_start_offset;
        t->memory.stack_top_paddr = t->memory.block_base + t->memory.stack_top_offset;
    } else {
//         // Allocate a dynamic block
//         t->memory.block_base = dalloc(p, t->memory.block_size);
        t->memory.block_base = 0x80000000ul;

        // Allocate and map physical memory
        ppfn_t ppfn = palloc(stack_size / PAGE_SIZE);
        paddr_t paddr = ppfn_to_paddr(ppfn);
        int mapped_count = get_hal_exports()->map_range(p->page_table,
            t->memory.block_base + t->memory.stack_limit_offset,
            paddr, stack_size, 1, 1, 1, 0, 0);

//
//         // Allocate memory and map it
//         // Msg send
//         ulong paddr = PFN_TO_ADDR(palloc(t->memory.msg_send_size / PAGE_SIZE));
//         assert(paddr);
//         int succeed = hal->map_user(
//             p->page_dir_pfn,
//             t->memory.block_base + t->memory.msg_send_offset,
//             paddr, t->memory.msg_send_size, 0, 1, 1, 0
//         );
//         assert(succeed);
//         t->memory.msg_send_paddr = paddr;
//         //kprintf("Mapped msg send, vaddr @ %p, paddr @ %p\n",
//         //        (void *)(t->memory.block_base + t->memory.msg_send_offset), (void *)paddr);
//
//         // Msg recv
//         paddr = PFN_TO_ADDR(palloc(t->memory.msg_recv_size / PAGE_SIZE));
//         assert(paddr);
//         succeed = hal->map_user(
//             p->page_dir_pfn,
//             t->memory.block_base + t->memory.msg_recv_offset,
//             paddr, t->memory.msg_recv_size, 0, 1, 1, 0
//         );
//         assert(succeed);
//         t->memory.msg_recv_paddr = paddr;
//         //kprintf("Mapped msg recv, vaddr @ %p, paddr @ %p\n",
//         //        (void *)(t->memory.block_base + t->memory.msg_recv_offset), (void *)paddr);
//
//         // TLS
//         paddr = PFN_TO_ADDR(palloc(tls_size / PAGE_SIZE));
//         assert(paddr);
//         succeed = hal->map_user(
//             p->page_dir_pfn,
//             t->memory.block_base + t->memory.tls_start_offset,
//             paddr, tls_size, 0, 1, 1, 0
//         );
//         assert(succeed);
//         t->memory.tls_start_paddr = paddr;
//         //kprintf("Mapped TLS, vaddr @ %p, paddr @ %p\n",
//         //       (void *)(t->memory.block_base + t->memory.tls_start_offset), (void *)paddr);
//
//         // Stack
//         paddr = PFN_TO_ADDR(palloc(stack_size / PAGE_SIZE));
//         assert(paddr);
//         succeed = hal->map_user(
//             p->page_dir_pfn,
//             t->memory.block_base + t->memory.stack_limit_offset,
//             paddr, stack_size, 0, 1, 1, 0
//         );
//         assert(succeed);
//         t->memory.stack_top_paddr = paddr + stack_size;
//         //kprintf("Mapped stack, vaddr @ %p, paddr @ %p\n",
//         //        (void *)(t->memory.block_base + t->memory.stack_limit_offset), (void *)paddr);
    }

    // Insert TCB into TLS
    //t->memory.tcb_start_offset = t->memory.tls_start_offset;
    //t->memory.tcb_start_paddr = t->memory.tls_start_paddr;
    //t->memory.tcb_size = sizeof(struct thread_control_block);

    //t->memory.tls_start_offset += t->memory.tcb_size;
    //t->memory.tls_start_paddr += t->memory.tcb_size;

    // Initialize TCB
    //struct thread_control_block *tcb = (struct thread_control_block *)t->memory.tcb_start_paddr;
    //kprintf("TCB @ %p\n", tcb);

    //tcb->self = (struct thread_control_block *)(t->memory.block_base + t->memory.tcb_start_offset);
    //tcb->msg_send = (void *)(t->memory.block_base + t->memory.msg_send_offset);
    //tcb->msg_recv = (void *)(t->memory.block_base + t->memory.msg_recv_offset);
    //tcb->tls = (void *)(t->memory.block_base + t->memory.tls_start_offset);
    //tcb->proc_id = p->proc_id;
    //tcb->thread_id = t->thread_id;

    // Prepare the param
    //ulong *param_ptr = (ulong *)(t->memory.stack_top_paddr - sizeof(ulong));
    //*param_ptr = param;

    // Context
    get_hal_exports()->init_context(
        &t->context, entry_point, param,
        t->memory.block_base + t->memory.stack_top_offset - sizeof(ulong) * 2,
        t->user_mode);

    // Init lock
    spinlock_init(&t->lock);
    t->ref_count.value = 2;
    atomic_inc(&p->ref_count.value);

    // Insert the thread into the thread list
    slist_push_back_exclusive(&p->threads, t);
    slist_push_back_exclusive(&threads, t);

    // Done
    return t->tid;
}


/*
 * Thread destruction
 */
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
    panic_if(!spinlock_is_locked(&t->lock), "thread must be locked!\n");
    memcpy(&t->context, ctxt, sizeof(struct reg_context));
}


/*
 * Change thread state
 */
void run_thread(struct thread *t)
{
    //assert(t->state == thread_enter || t->state == thread_wait || t->state == thread_stall || t->state == thread_sched);

    panic_if(!spinlock_is_locked(&t->lock), "thread must be locked!\n");

    t->state = THREAD_STATE_NORMAL;
    sched_put(t);
}
