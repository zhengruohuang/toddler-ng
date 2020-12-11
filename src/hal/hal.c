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
static char loader_arch_args[32] = { 0 };
static struct loader_args loader_args = { 0 };

struct loader_args *get_loader_args()
{
    return &loader_args;
}


/*
 * Arch funcs
 */
static struct hal_arch_funcs arch_funcs = { 0 };

struct hal_arch_funcs *get_hal_arch_funcs()
{
    return &arch_funcs;
}


/*
 * Init steps
 */
static void init_libk()
{
    if (arch_funcs.init_libk) {
        arch_funcs.init_libk();
    }

    kprintf("In HAL!\n");
    init_libk_memmap(loader_args.memmap);
}

static void init_arch()
{
    if (arch_funcs.init_arch) {
        arch_funcs.init_arch();
    }
}

static void init_arch_mp()
{
    if (arch_funcs.init_arch_mp) {
        arch_funcs.init_arch_mp();
    }
}

static void arch_init_int()
{
    if (arch_funcs.init_int) {
        arch_funcs.init_int();
    }
}

static void arch_init_int_mp()
{
    if (arch_funcs.init_int_mp) {
        arch_funcs.init_int_mp();
    }
}

static void arch_init_mm()
{
    if (arch_funcs.init_mm) {
        arch_funcs.init_mm();
    }
}

static void arch_init_mm_mp()
{
    if (arch_funcs.init_mm_mp) {
        arch_funcs.init_mm_mp();
    }
}


/*
 * Per-arch wrappers
 */
ulong arch_hal_direct_access(paddr_t paddr, int count, int cache)
{
    if (arch_funcs.hal_direct_access) {
        return arch_funcs.hal_direct_access(paddr, count, cache);
    }

    panic("Arch does not HAL direct access!\n");
    return 0;
}

ulong arch_get_cur_mp_id()
{
    if (arch_funcs.get_cur_mp_id) {
        return arch_funcs.get_cur_mp_id();
    }

    panic_if(get_num_cpus() != 1,
             "MP arch HAL must implement get_cpu_index!");
    return 0;
}

void arch_start_cpu(int mp_seq, ulong mp_id, ulong entry)
{
    if (arch_funcs.start_cpu) {
        arch_funcs.start_cpu(mp_seq, mp_id, entry);
        return;
    }

    panic_if(get_num_cpus() != 1,
             "MP arch HAL must implement bringup_secondary_cpu!");
}

void arch_register_drivers()
{
    if (arch_funcs.register_drivers) {
        arch_funcs.register_drivers();
    }
}

ulong arch_get_syscall_params(struct reg_context *regs, ulong *param0, ulong *param1, ulong *param2)
{
    if (arch_funcs.get_syscall_params) {
        return arch_funcs.get_syscall_params(regs, param0, param1, param2);
    }

    panic("Arch HAL must implement arch_get_syscall_params!");
    return 0;
}

void arch_set_syscall_return(struct reg_context *regs, int success, ulong return0, ulong return1)
{
    if (arch_funcs.set_syscall_return) {
        arch_funcs.set_syscall_return(regs, success, return0, return1);
        return;
    }

    panic("Arch HAL must implement arch_set_syscall_return!");
}

int arch_handle_syscall(ulong num, ulong param0, ulong param1, ulong param2, ulong *return0, ulong *return1)
{
    if (arch_funcs.handle_arch_syscall) {
        return arch_funcs.handle_arch_syscall(num, param0, param1, param2, return0, return1);
    }

    return 1;
}

void arch_disable_local_int()
{
    if (arch_funcs.arch_disable_local_int) {
        arch_funcs.arch_disable_local_int();
        return;
    }

    panic("Arch HAL must implement disable_local_int!");
}

void arch_enable_local_int()
{
    if (arch_funcs.arch_enable_local_int) {
        return arch_funcs.arch_enable_local_int();
    }

    panic("Arch HAL must implement enable_local_int!");
}

void arch_switch_to(ulong thread_id, struct reg_context *context,
                    void *page_table, int user_mode, ulong asid, ulong tcb)
{
    if (arch_funcs.switch_to) {
        arch_funcs.switch_to(thread_id, context, page_table, user_mode, asid, tcb);
        return;
    }

    panic("Arch HAL must implement switch_to!");
}

void arch_kernel_pre_dispatch(ulong sched_id, struct kernel_dispatch *kdi)
{
    if (arch_funcs.kernel_pre_dispatch) {
        return arch_funcs.kernel_pre_dispatch(sched_id, kdi);
    }
}

void arch_kernel_post_dispatch(ulong sched_id, struct kernel_dispatch *kdi)
{
    if (arch_funcs.kernel_post_dispatch) {
        return arch_funcs.kernel_post_dispatch(sched_id, kdi);
    }
}


/*
 * Common entry
 */
void hal(struct loader_args *largs, struct hal_arch_funcs *funcs)
{
    // BSS
    init_bss();

    // Save largs and funcs
    memcpy(&loader_args, largs, sizeof(struct loader_args));
    memcpy(&arch_funcs, funcs, sizeof(struct hal_arch_funcs));

    // Save arch args
    if (largs->arch_args && largs->arch_args_bytes <= sizeof(loader_arch_args)) {
        memcpy(loader_arch_args, largs->arch_args, largs->arch_args_bytes);
        loader_args.arch_args = loader_arch_args;
    }

    // Init libk and arch
    init_libk();
    init_arch();

    // Check if we were able to save loader arch args
    panic_if(largs->arch_args_bytes > sizeof(loader_arch_args),
             "No enough space to save arch_args!\n");

    // Init pre-kernel palloc
    init_pre_palloc();
    init_mem_map();
    arch_init_mm();

    // Init int seq and default handlers
    init_int_seq();
    init_int_handler();
    init_syscall();

    // Init devices
    init_dev();

    // Init CPU
    init_topo();
    init_per_cpu_area();

    // Init interrupt
    init_context();
    init_int_state();
    arch_init_int();

    // Init kernel
    init_kernel();

    // Bring up secondary processors
    bringup_all_secondary_cpus();

    // Unlock all secondary CPUs
    release_secondary_cpu_lock();

    // Test kernel phase1
    kernel_test_phase1();

    // Start all devices
    start_all_devices();

    // And finally, start working
    kernel_start();

    kprintf("HAL done!\n");
    unreachable();
}

void hal_mp()
{
    // Init arch
    init_arch_mp();

    // Init MM
    arch_init_mm_mp();

    // Init interrupt
    init_context_mp();
    init_int_state_mp();
    arch_init_int_mp();

    // Init done, wait for the global signal to start working
    secondary_cpu_init_done();

    // Test kernel phase1
    kernel_test_phase1();

    // Start all devices
    start_all_devices_mp();

    // And finally, start working
    kernel_start();

    kprintf("HAL MP done!\n");
    unreachable();
}
