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
 * Start
 */
static void first_thread(ulong param)
{
    int seq = hal_get_cur_mp_seq();
    kprintf("First thread on CPU #%d!\n", seq);

    // Terminate self
    syscall_thread_exit_self(0);
}

static void start()
{
    // Create and switch to the first thread on that CPU
    // This is especially important for the bootstrap CPU as it safely switches
    // the BSP's stack
    struct thread *th = NULL;
    create_and_access_kernel_thread(tid, t, &first_thread, 0, NULL) {
        t->state = THREAD_STATE_NORMAL;
        atomic_inc(&t->ref_count.value);
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
    hal_exp->kernel->palloc = palloc_direct_mapped;
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

void acquire_kprintf()
{
    spinlock_lock_int(&kprintf_lock);
}

void release_kprintf()
{
    spinlock_unlock_int(&kprintf_lock);
}

int kprintf(const char *fmt, ...)
{
    int ret = 0;

    va_list va;
    va_start(va, fmt);

    acquire_kprintf();
    ret = __vkprintf(fmt, va);
    release_kprintf();

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

u64 hal_get_ms()
{
    return hal->get_ms();
}

void hal_set_syscall_return(struct reg_context *context, int success, ulong return0, ulong return1)
{
    return hal->set_syscall_return(context, success, return0, return1);
}

void *hal_access_per_cpu_var(int *offset, size_t size)
{
    return hal->access_per_cpu_var(offset, size);
}

void *hal_cast_paddr_to_kernel_ptr(paddr_t paddr)
{
    if (hal->has_direct_access) {
        return (void *)hal->direct_paddr_to_vaddr(paddr, 0, 1);
    } else {
        return (void *)cast_paddr_to_vaddr(paddr);
    }
}

paddr_t hal_cast_kernel_ptr_to_paddr(void *ptr)
{
    if (hal->has_direct_access) {
        return hal->direct_vaddr_to_paddr((ulong)ptr, 0);
    } else {
        return cast_ptr_to_paddr(ptr);
    }
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
    open_libk_devtree(hal_exp->devtree);

    kprintf("In kernel!\n");

    reserve_pfndb();
    reserve_palloc();

    init_pfndb();
    init_palloc();
    init_malloc();

    // TODO: salloc need to check for uninitialized obj
    init_dispatch();
    init_sched();
    init_vm();
    init_asid();
    init_process();
    init_tlb_shootdown();
    init_thread();
    init_wait();
    init_ipc();
    init_startup();

    init_test();
    init_kexp(hal_exp);

    kprintf("Kernel init done\n");
}
