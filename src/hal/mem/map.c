#include "common/include/inttypes.h"
#include "hal/include/hal.h"
#include "hal/include/mem.h"
#include "hal/include/kernel.h"


static void *hal_page_table;
static generic_map_range_t generic_map_range;


int hal_map_range(ulong vaddr, ulong paddr, ulong size, int cache)
{
    return generic_map_range(hal_page_table, vaddr, paddr, size,
                             cache, 0, 1, 1, 0, pre_palloc);
}

int kernel_map_range(void *page_table, ulong vaddr, ulong paddr, size_t length,
                     int cacheable, int exec, int write, int kernel,
                     int override)
{
    return generic_map_range(page_table, vaddr, paddr, length,
                             cacheable, exec, write, kernel, override,
                             kernel_palloc);
}

void init_mem_map()
{
    struct loader_args *largs = get_loader_args();
    struct hal_arch_funcs *funcs = get_hal_arch_funcs();

    hal_page_table = largs->page_table;
    generic_map_range = funcs->map_range;
}
