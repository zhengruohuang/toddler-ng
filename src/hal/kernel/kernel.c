#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "hal/include/export.h"
#include "hal/include/hal.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/kernel.h"
#include "hal/include/mp.h"
#include "hal/include/int.h"
#include "hal/include/mem.h"
#include "hal/include/dev.h"


/*
 * Kernel export wrappers
 */
static struct kernel_exports kexp = { };

ppfn_t kernel_palloc(int count)
{
    return kexp.palloc(count);
}

int kernel_pfree(ppfn_t ppfn)
{
    return kexp.pfree(ppfn);
}

void *kernel_palloc_ptr(int count)
{
    return kexp.palloc_ptr(count);
}

int kernel_pfree_ptr(void *ptr)
{
    return kexp.pfree_ptr(ptr);
}

void kernel_dispatch(ulong sched_id, struct kernel_dispatch *kdi)
{
    arch_kernel_pre_dispatch(sched_id, kdi);
    kexp.dispatch(sched_id, kdi);
    arch_kernel_post_dispatch(sched_id, kdi);
}

void kernel_start()
{
    kexp.start();
}

void kernel_test_phase1()
{
    if (kexp.test_phase1) {
        kexp.test_phase1();
    }
}


/*
 * HAL exports and kernel init
 */
static struct hal_exports hexp = { .kernel = &kexp };

static void fill_hal_exports()
{
    struct loader_args *largs = get_loader_args();
    struct hal_arch_funcs *funcs = get_hal_arch_funcs();

//     // Alloc structs memory
//     hexp = (struct hal_exports *)mempool_alloc(sizeof(struct hal_exports));
//     kexp = (struct kernel_exports *)mempool_alloc(sizeof(struct kernel_exports));
//     hexp.kernel = kexp;

    // General functions
    hexp.putchar = funcs->putchar;
    hexp.time = NULL;
    hexp.halt = halt_all_cpus;

    // Kernel info
    hexp.kernel_page_table = largs->page_table;

    // Device tree
    hexp.devtree = largs->devtree;

    // Core image info
    hexp.coreimg = largs->coreimg;

    // MP
    hexp.num_cpus = get_num_cpus();
    hexp.get_cur_mp_id = funcs->get_cur_mp_id;
    hexp.get_cur_mp_seq = get_cur_mp_seq;
    hexp.access_per_cpu_var = access_per_cpu_var;

    // Physical memory map
    hexp.memmap = get_memmap();

    // Interrupt
    hexp.disable_local_int = disable_local_int;
    hexp.enable_local_int = enable_local_int;
    hexp.restore_local_int = restore_local_int;

    // User-space interrupt handler
    hexp.int_register = user_int_register;
    hexp.int_eoi = user_int_eoi;

    // Clock
    hexp.get_ms = clock_get_ms;

    // Mapping
    hexp.map_range = kernel_map_range;
    hexp.unmap_range = funcs->unmap_range;
    hexp.translate = funcs->translate;

    // Address space
    //hexp.user_page_dir_page_count = 0;     // TODO: funcs->exports.user_page_dir_page_count;
    hexp.vaddr_space_end = 0;  // TODO: funcs->exports.vaddr_space_end;
    hexp.init_addr_space = funcs->init_addr_space;

    // Context
    hexp.init_context = funcs->init_context;
    hexp.set_context_param = funcs->set_context_param;
    hexp.switch_context = switch_context;
    hexp.set_syscall_return = funcs->set_syscall_return;

    // TLB
    hexp.invalidate_tlb = funcs->invalidate_tlb;
}

static void call_kernel_entry()
{
    struct loader_args *largs = get_loader_args();
    kernel_entry_t kernel_entry = largs->kernel_entry;

    kprintf("\tKernel entry: %p\n", kernel_entry);
    kernel_entry(&hexp);
    kprintf("Kernel has been initialized!\n");
}

void init_kernel()
{
    fill_hal_exports();
    call_kernel_entry();
}
