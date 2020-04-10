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


/*
 * Kernel export wrappers
 */
static struct kernel_exports *kexp;

ppfn_t kernel_palloc_tag(int count, int tag)
{
    return kexp->palloc_tag(count, tag);
}

ppfn_t kernel_palloc(int count)
{
    return kexp->palloc(count);
}

int kernel_pfree(ppfn_t ppfn)
{
    return kexp->pfree(ppfn);
}

void kernel_dispatch(ulong sched_id, struct kernel_dispatch_info *kdi)
{
    arch_kernel_dispatch_prep(sched_id, kdi);
    return kexp->dispatch(sched_id, kdi);
}


/*
 * HAL exports and kernel init
 */
static struct hal_exports *hexp;
static void (*kernel_entry)(struct hal_exports *exp);

static void fill_hal_exports()
{
    struct loader_args *largs = get_loader_args();
    struct hal_arch_funcs *funcs = get_hal_arch_funcs();

    // Alloc structs memory
    hexp = (struct hal_exports *)mempool_alloc(sizeof(struct hal_exports));
    kexp = (struct kernel_exports *)mempool_alloc(sizeof(struct kernel_exports));
    hexp->kernel = kexp;

    // General functions
    hexp->putchar = funcs->putchar;
    hexp->time = NULL;
    hexp->halt = halt_all_cpus;

    // Kernel info
    hexp->kernel_page_table = largs->page_table;

    // Core image info
    hexp->coreimg_load_addr = 0;    // TODO: get_bootparam()->coreimg_load_addr;

    // MP
    hexp->num_cpus = get_num_cpus();
    hexp->get_cur_mp_id = funcs->get_cur_mp_id;
    hexp->get_cur_mp_seq = get_cur_mp_seq;

    // Physical memory map
    hexp->memmap = get_memmap();

    // Interrupt
    hexp->disable_local_int = disable_local_int;
    hexp->enable_local_int = enable_local_int;
    hexp->restore_local_int = restore_local_int;

    // Mapping
    hexp->map_range = kernel_map_range;
    hexp->unmap_range = funcs->unmap_range;
    hexp->translate = funcs->translate;

    // Address space
    hexp->user_page_dir_page_count = 0;     // TODO: funcs->exports.user_page_dir_page_count;
    hexp->vaddr_space_end = 0;  // TODO: funcs->exports.vaddr_space_end;
    hexp->init_addr_space = funcs->init_addr_space;

    // Context
    hexp->init_context = funcs->init_context;
    hexp->set_context_param = funcs->set_context_param;
    hexp->switch_context = switch_context;
    hexp->set_syscall_return = funcs->set_syscall_return;

    // TLB
    hexp->invalidate_tlb = funcs->invalidate_tlb;
}

static void call_kernel_entry()
{
    struct loader_args *largs = get_loader_args();
    kernel_entry = largs->kernel_entry;

    kprintf("\tKernel entry: %p\n", kernel_entry);
    kernel_entry(hexp);
    kprintf("Kernel has been initialized!\n");
}

void init_kernel()
{
    fill_hal_exports();
    call_kernel_entry();
}
