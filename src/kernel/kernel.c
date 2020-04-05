#include "common/include/inttypes.h"
#include "hal/include/export.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/atomic.h"
#include "kernel/include/lib.h"
#include "kernel/include/mem.h"


/*
 * Dispatch
 */
static void dispatch(ulong sched_id, struct kernel_dispatch_info *kdi)
{
}


/*
 * Kernel exports
 */
static ulong wrap_palloc_tag(int count, int tag)
{
    return 0;
//     return palloc_tag(count, tag);
}

static ulong wrap_palloc(int count)
{
    return 0;
//     return palloc(count);
}

static int wrap_pfree(ulong pfn)
{
    return 0;
//     return pfree(pfn);
}

static void init_kexp(struct hal_exports *hal_exp)
{
    hal_exp->kernel->dispatch = dispatch;
    hal_exp->kernel->palloc_tag = wrap_palloc_tag;
    hal_exp->kernel->palloc = wrap_palloc;
    hal_exp->kernel->pfree = wrap_pfree;
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
void kernel_entry(struct hal_exports *hal_exp)
{
    hal = hal_exp;

    spinlock_init(&kprintf_lock);
    init_libk_putchar(hal_exp->putchar);
    init_libk_stop(hal_stop);

    kprintf("In kernel!\n");

    init_pfndb();
    init_palloc();

    init_kexp(hal_exp);
}
