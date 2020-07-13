#include "common/include/inttypes.h"
#include "hal/include/export.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/atomic.h"
#include "kernel/include/lib.h"
#include "kernel/include/mem.h"
#include "kernel/include/struct.h"
#include "kernel/include/proc.h"
#include "kernel/include/syscall.h"
#include "kernel/include/test.h"
#include "kernel/include/kernel.h"


/*
 * Dispatch
 */
static syscall_handler_t handlers[MAX_NUM_SYSCALLS];

static void init_dispatch()
{
    memzero(handlers, sizeof(handlers));

    handlers[SYSCALL_YIELD] = syscall_handler_yield;
    handlers[SYSCALL_INTERRUPT] = syscall_handler_interrupt;
    handlers[SYSCALL_THREAD_EXIT] = syscall_handler_thread_exit;
}

static void dispatch(ulong thread_id, struct kernel_dispatch *kdi)
{
    //kprintf("dispatch: %d\n", kdi->num);

    int do_sched = 0;

    int valid = 0;
    thread_access_exclusive(thread_id, t) {
        process_access_exclusive(t->pid, p) {
            valid = 1;

            // TODO: check thread/process state

            syscall_handler_t handler = syscall_handler_illegal;
            if (kdi->num < MAX_NUM_SYSCALLS && handlers[kdi->num]) {
                handler = handlers[kdi->num];
            }

            int save_ctxt = 0;
            int put_back = 0;
            int handle_type = handler(p, t, kdi);
            switch (handle_type) {
            case SYSCALL_HANDLER_SAVE_RESCHED:
                save_ctxt = 1;
            case SYSCALL_HANDLER_RESCHED:
                do_sched = 1;
                put_back = 1;
                break;

            case SYSCALL_HANDLER_SAVE_SKIP:
                save_ctxt = 1;
            case SYSCALL_HANDLER_SKIP:
                do_sched = 1;
                break;

            case SYSCALL_HANDLER_SAVE_CONTINUE:
                save_ctxt = 1;
            case SYSCALL_HANDLER_CONTINUE:
            default:
                break;
            }

            if (save_ctxt) {
                thread_save_context(t, kdi->regs);
            }

            if (put_back) {
                sched_put(t);
            }
        }
    }

    panic_if(!valid, "Invalid tid: %lx\n", kdi->tid);
    if (do_sched) {
        sched();
    }
}


/*
 * Start
 */
static void first_thread(ulong param)
{
    int seq = hal_get_cur_mp_seq();
    kprintf("First thread on CPU #%d!\n", seq);

    // Terminate self
    syscall_thread_exit_self();
}

static void start()
{
    // Create and switch to the first thread on that CPU
    // This is especially important for the bootstrap CPU as it safely switches
    // the BSP's stack
    struct thread *th = NULL;
    create_kthread(tid, t, &first_thread, 0, -1) {
        t->state = THREAD_STATE_NORMAL;
        th = t;
    }

    switch_to_thread(th);
    unreachable();
}


/*
 * Kernel exports
 */
static void init_kexp(struct hal_exports *hal_exp)
{
    hal_exp->kernel->palloc = palloc;
    hal_exp->kernel->pfree = pfree;

    hal_exp->kernel->palloc_ptr = palloc_ptr;
    hal_exp->kernel->pfree_ptr = pfree_ptr;

    hal_exp->kernel->dispatch = dispatch;
    hal_exp->kernel->start = start;

    hal_exp->kernel->test_phase1 = test_mem;
}


/*
 * kprintf
 */
static spinlock_t kprintf_lock = SPINLOCK_INIT;

int kprintf(const char *fmt, ...)
{
    int ret = 0;

    va_list va;
    va_start(va, fmt);

    spinlock_lock_int(&kprintf_lock);
    ret = __vkprintf(fmt, va);
    spinlock_unlock_int(&kprintf_lock);

    va_end(va);

    return ret;
}

int kprintf_unlocked(const char *fmt, ...)
{
    int ret = 0;

    va_list va;
    va_start(va, fmt);
    ret = __vkprintf(fmt, va);
    va_end(va);

    return ret;
}


/*
 * HAL exports
 */
static struct hal_exports *hal;

struct hal_exports *get_hal_exports()
{
    return hal;
}

int hal_get_num_cpus()
{
    return hal ? hal->num_cpus : 1;
}

ulong hal_get_cur_mp_seq()
{
    if (hal && hal->get_cur_mp_seq) {
        return hal->get_cur_mp_seq();
    }

    return 0;
}

void hal_stop()
{
    if (hal && hal->halt) {
        hal->halt(0);
    }

    while (1);
}

int hal_disable_local_int()
{
    return hal->disable_local_int();
}

void hal_enable_local_int()
{
    hal->enable_local_int();
}

int hal_restore_local_int(int enabled)
{
    return hal->restore_local_int(enabled);
}


/*
 * Kernel entry
 */
void kernel(struct hal_exports *hal_exp)
{
    hal = hal_exp;

    spinlock_init(&kprintf_lock);
    init_libk_putchar(hal_exp->putchar);
    init_libk_memmap(hal_exp->memmap);
    init_libk_stop(hal_stop);
    init_libk_coreimg(hal_exp->coreimg);

    kprintf("In kernel!\n");

    reserve_pfndb();
    reserve_palloc();

    init_pfndb();
    init_palloc();
    init_salloc();
    init_malloc();

    init_slist();

    init_dispatch();
    init_sched();
    init_process();
    init_thread();
    init_startup();

    init_test();
    init_kexp(hal_exp);
}
