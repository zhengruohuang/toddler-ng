#include "common/include/inttypes.h"
#include "loader/include/export.h"
#include "hal/include/hal.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/mp.h"
#include "hal/include/int.h"
#include "hal/include/mem.h"
#include "hal/include/kernel.h"
#include "hal/include/dev.h"


/*
 * BSS
 */
extern int __bss_start;
extern int __bss_end;

static void init_bss()
{
    int *cur;

    for (cur = &__bss_start; cur < &__bss_end; cur++) {
        *cur = 0;
    }
}


/*
 * Loader info
 */
static struct loader_args *loader_args = NULL;

struct loader_args *get_loader_args()
{
    return loader_args;
}


/*
 * Arch funcs
 */
static struct hal_arch_funcs *arch_funcs;

struct hal_arch_funcs *get_hal_arch_funcs()
{
    return arch_funcs;
}


/*
 * Init steps
 */
static void init_libk()
{
    if (arch_funcs && arch_funcs->init_libk) {
        arch_funcs->init_libk();
    }

    init_libk_memmap(loader_args->memmap, loader_args->num_memmap_entries,
                     loader_args->num_memmap_limit);

    kprintf("In HAL!\n");
}

static void init_arch()
{
    if (arch_funcs && arch_funcs->init_arch) {
        arch_funcs->init_arch();
    }
}

static void init_arch_secondary()
{
    if (arch_funcs && arch_funcs->init_arch) {
        arch_funcs->init_arch();
    }
}


/*
 * Per-arch wrappers
 */
ulong arch_hal_direct_access(ulong paddr, int count, int cache)
{
    if (arch_funcs->hal_direct_access) {
        return arch_funcs->hal_direct_access(paddr, count, cache);
    }

    panic("Arch does not HAL direct access!\n");
    return 0;
}

int arch_get_cpu_index()
{
    if (arch_funcs->get_cur_cpu_index) {
        return arch_funcs->get_cur_cpu_index();
    }

    panic_if(get_num_cpus() != 1,
             "MP arch HAL must implement get_cpu_index!");
    return 0;
}

void arch_bringup_cpu(int index)
{
    if (arch_funcs->bringup_cpu) {
        arch_funcs->bringup_cpu(index);
    }

    panic_if(get_num_cpus() != 1,
             "MP arch HAL must implement bringup_secondary_cpu!");
}

void arch_register_drivers()
{
    if (arch_funcs->register_drivers) {
        arch_funcs->register_drivers();
    }
}

ulong arch_get_syscall_params(struct reg_context *regs, ulong *param0, ulong *param1, ulong *param2)
{
    return 0;
}

void arch_set_syscall_return(struct reg_context *regs, int success, ulong return0, ulong return1)
{
}

int arch_handle_syscall(ulong num, ulong param0, ulong param1, ulong param2, ulong *return0, ulong *return1)
{
    if (arch_funcs->handle_arch_syscall) {
        return arch_funcs->handle_arch_syscall(num, param0, param1, param2, return0, return1);
    }

    return 1;
}

void arch_disable_local_int()
{
    if (arch_funcs->arch_disable_local_int) {
        arch_funcs->arch_disable_local_int();
        return;
    }

    panic("Arch HAL must implement disable_local_int!");
}

void arch_enable_local_int()
{
    if (arch_funcs->arch_disable_local_int) {
        return arch_funcs->arch_disable_local_int();
    }

    panic("Arch HAL must implement enable_local_int!");
}

void arch_switch_to(ulong sched_id, struct reg_context *context,
    ulong page_dir_pfn, int user_mode, ulong asid, ulong tcb)
{
    if (arch_funcs->switch_to) {
        arch_funcs->switch_to(sched_id, context, page_dir_pfn, user_mode, asid, tcb);
    }

    panic("Arch HAL must implement switch_to!");
}

void arch_kernel_dispatch_prep(ulong sched_id, struct kernel_dispatch_info *kdi)
{
    if (arch_funcs->kernel_dispatch_prep) {
        return arch_funcs->kernel_dispatch_prep(sched_id, kdi);
    }
}


/*
 * Common entry
 */
void hal_entry_primary(struct loader_args *largs, struct hal_arch_funcs *funcs)
{
    // BSS
    init_bss();

    // Save largs and funcs
    loader_args = largs;
    arch_funcs = funcs;

    // Init libk and arch
    init_libk();
    init_arch();

    // Init pre-kernel palloc
    init_pre_palloc();
    init_mem_map();

    // Init CPU
    init_topo();
    init_per_cpu_area();
    init_halt();

    // Init multi task context
    init_context();

    // Init interrupt
    init_int_state();
    init_int_seq();
    init_syscall();

    // Init devices
    init_dev();

    // Init kernel
    init_kernel();

    // Bring up secondary processors
    bringup_all_secondary_cpus();

    // Unlock all secondary CPUs
    release_secondary_cpu_lock();

    // Start all devices
    start_all_devices();

    // And finally, enable local interrupts
    //enable_local_int();

    kprintf("HAL done!\n");

    while (1);
}

void hal_entry_secondary()
{
    // Init arch
    init_arch_secondary();

    // Init multi task context
    init_context();

    // Init interrupt
    init_int_state();

    // Init done, wait for the global signal to start working
    secondary_cpu_init_done();

    // Start working

    // And finally, enable local interrupts
    enable_local_int();

    while (1);
}
