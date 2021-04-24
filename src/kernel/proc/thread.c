/*
 * Thread manager
 */


#include "common/include/compiler.h"
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
__unused_func static void kernel_demo_thread(ulong param)
{
    do {
        __unused_var int index = param;
        __unused_var int cpu_id = hal_get_cur_mp_seq();
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

    return t;
}

void release_thread(struct thread *t)
{
    panic_if(!t, "Inconsistent acquire/release pair\n");
    panic_if(t->ref_count.value <= 0, "Inconsistent ref_count: %ld, tid: %lx\n",
             t->ref_count.value, t->tid);

    ref_count_dec(&t->ref_count);
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

ulong get_num_threads()
{
    ulong num = 0;

    list_access_exclusive(&threads) {
        num = threads.count;
    }

    return num;
}


/*
 * Init
 */
static salloc_obj_t thread_salloc_obj;

void init_thread()
{
    kprintf("Initializing thread manager\n");

    // Create salloc obj
    salloc_create_default(&thread_salloc_obj, "thread", sizeof(struct thread));

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

static int thread_block_mapper(struct process *p, struct thread *t, struct vm_block *b)
{
    // Allocate physical memory
    t->memory.tls.start_paddr_ptr = palloc_ptr(1);
    void *initial_stack_paddr_ptr = palloc_ptr(1);
    t->memory.stack.top_paddr_ptr = initial_stack_paddr_ptr + PAGE_SIZE;

    ref_count_inc(&p->vm.num_palloc_pages);
    ref_count_inc(&p->vm.num_palloc_pages);

    b->thread_map.tls_start_ptr = t->memory.tls.start_paddr_ptr;
    b->thread_map.stack_top_ptr = t->memory.stack.top_paddr_ptr;

    // Map virtual to physical
    spinlock_exclusive_int(&p->page_table_lock) {
        get_hal_exports()->map_range(p->page_table,
                                     b->base + t->memory.tls.start_offset,
                                     hal_cast_kernel_ptr_to_paddr(t->memory.tls.start_paddr_ptr),
                                     PAGE_SIZE, //t->memory.tls.size,
                                     1, 1, 1, 0, 0);

        get_hal_exports()->map_range(p->page_table,
                                     b->base + t->memory.stack.top_offset - PAGE_SIZE,
                                     hal_cast_kernel_ptr_to_paddr(t->memory.stack.top_paddr_ptr - PAGE_SIZE),
                                     PAGE_SIZE,
                                     1, 1, 1, 0, 0);
    }

    return 0;
}

static int thread_block_reuse(struct process *p, struct thread *t, struct vm_block *b)
{
    t->memory.tls.start_paddr_ptr = b->thread_map.tls_start_ptr;
    t->memory.stack.top_paddr_ptr = b->thread_map.stack_top_ptr;

    return 0;
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
        t->memory.block = palloc_ptr(t->memory.block_size / PAGE_SIZE);
        t->memory.block_base = (ulong)t->memory.block;
        //kprintf("Kernel thread block allocated paddr @ %p\n", (void *)t->memory.block_base);

        t->memory.tls.start_paddr_ptr = (void *)(t->memory.block_base + t->memory.tls.start_offset);
        t->memory.stack.top_paddr_ptr = (void *)(t->memory.block_base + t->memory.stack.top_offset);

        set_stack_magic(t->memory.block_base + t->memory.stack.limit_offset);
    } else {
        // Allocate and map a dynamic block
        t->memory.block = vm_alloc_thread(p, t, thread_block_mapper, thread_block_reuse);
        t->memory.block_base = t->memory.block->base;
    }

    // Adjust stack top to make some room for msg block
    const ulong msg_block_size = MAX_MSG_SIZE;
    t->memory.msg.size = msg_block_size;
    t->memory.msg.start_offset = t->memory.tls.start_offset;
    t->memory.msg.start_paddr_ptr = t->memory.tls.start_paddr_ptr;
    t->memory.tls.start_offset += msg_block_size;
    t->memory.tls.start_paddr_ptr += msg_block_size;

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

    // FIXME: Push param to stack, not needed by most arch
    // need to make this arch-specific
    ulong *stack_param_paddr_ptr = (ulong *)t->memory.stack.top_paddr_ptr;
    *(stack_param_paddr_ptr - 1) = param;

    // Context
    get_hal_exports()->init_context(
        &t->context, entry_point, param,
        t->memory.block_base + t->memory.stack.top_offset,
        t->memory.block_base + t->memory.tls.start_offset,
        t->user_mode);

    // Sched class
    t->sched_class = SCHED_CLASS_NORMAL;

    //kprintf("Stack top @ %lx\n", t->memory.block_base + t->memory.stack.top_offset);

    // Done
    return t->tid;
}


/*
 * Exit thread
 */
void exit_thread(struct thread *t)
{
    //kprintf("Exiting thread @ %p\n", t);

    access_process(t->pid, proc) {
        // TODO: check thread state

        // Clean up dynamic area
        if (proc->type == PROCESS_TYPE_KERNEL) {
            check_stack_magic(t->memory.block_base + t->memory.stack.limit_offset);
            pfree_ptr((void *)t->memory.block_base);
        } else {
            vm_free_block(proc, t->memory.block);
        }

        access_wait_queue_exclusive(wait_queue) {
            list_remove_exclusive(&threads, &t->node);
            list_remove_exclusive(&proc->threads, &t->node_proc);
        }

        ref_count_dec(&t->ref_count);
        panic_if(!ref_count_is_zero(&t->ref_count),
                 "Inconsistent ref count: %ld, tid @ %lx\n",
                 t->ref_count.value, t->tid);

        sfree(t);
        ref_count_dec(&proc->ref_count);
    }
}


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
