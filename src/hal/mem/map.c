#include "common/include/inttypes.h"
#include "hal/include/hal.h"
#include "hal/include/mem.h"
#include "hal/include/kernel.h"


static void *hal_page_table;
static map_range_t arch_hal_map_range, arch_kernel_map_range;
static page_translate_t arch_translate;


int hal_map_range(ulong vaddr, paddr_t paddr, ulong size, int cache, int kernel)
{
    return arch_hal_map_range(hal_page_table, vaddr, paddr, size,
                              cache, 1, 1, kernel, 0);
}

paddr_t hal_translate(ulong vaddr)
{
    return arch_translate(hal_page_table, vaddr);
}


void init_mem_map()
{
    struct loader_args *largs = get_loader_args();
    struct hal_arch_funcs *funcs = get_hal_arch_funcs();

    hal_page_table = largs->page_table;
    arch_hal_map_range = funcs->hal_map_range;
    arch_kernel_map_range = funcs->kernel_map_range;
    arch_translate = funcs->translate;

    panic_if(!arch_hal_map_range, "hal_map_range must be supplied\n");
    panic_if(!arch_kernel_map_range, "kernel_map_range must be supplied\n");
    panic_if(!arch_translate, "translate must be supplied\n");
}
