#include "common/include/inttypes.h"
#include "hal/include/export.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/atomic.h"
#include "kernel/include/lib.h"
#include "kernel/include/mem.h"
#include "kernel/include/test.h"


/*
 * Dispatch
 */
static void dispatch(ulong sched_id, struct kernel_dispatch_info *kdi)
{
}


/*
 * Kernel exports
 */
static ppfn_t wrap_palloc(int count)
{
    return palloc_direct_mapped(count);
}

static int wrap_pfree(ppfn_t ppfn)
{
    return pfree(ppfn);
}

static void init_kexp(struct hal_exports *hal_exp)
{
    hal_exp->kernel->dispatch = dispatch;
    hal_exp->kernel->palloc = wrap_palloc;
    hal_exp->kernel->pfree = wrap_pfree;
    hal_exp->kernel->test_phase1 = test_mem;
}


/*
 * kprintf
 */
static spinlock_t kprintf_lock;

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

    kprintf("In kernel!\n");

    reserve_pfndb();
    reserve_palloc();

    init_pfndb();
    init_palloc();
    init_salloc();
    init_malloc();

    init_test();
    init_kexp(hal_exp);
}
