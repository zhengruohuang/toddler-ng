#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "hal/include/hal.h"
#include "hal/include/lib.h"
#include "hal/include/kprintf.h"
#include "hal/include/mem.h"


static int sysarea_grows_up = 0;
static ulong sysarea_lower = 0;
static ulong sysarea_upper = 0;


ulong get_sysarea_range(ulong *lower, ulong *upper)
{
    if (lower) *lower = sysarea_lower;
    if (upper) *upper = sysarea_upper;

    return sysarea_upper - sysarea_lower;
}

ppfn_t pre_palloc(int count)
{
    ulong size = count * PAGE_SIZE;
    ulong align = PAGE_SIZE;

    u64 paddr = find_free_memmap_region_for_palloc(size, align);
    ppfn_t ppfn = (ulong)paddr_to_ppfn(paddr);

    kprintf("pre_palloc @ %llx\n", paddr);
    return ppfn;

    //ulong paddr = (ulong)find_free_memmap_region(count * PAGE_SIZE, PAGE_SIZE);
    //ulong pfn = ADDR_TO_PFN(paddr);

    //kprintf("pre_palloc @ %lx\n", paddr);

    //return pfn;
}

ulong pre_valloc(int count, paddr_t paddr, int cache)
{
    struct hal_arch_funcs *arch_funcs = get_hal_arch_funcs();

    if (arch_funcs->has_direct_access) {
        return arch_hal_direct_access(paddr, count, cache);
    } else {
        ulong vaddr = 0;
        ulong size = PAGE_SIZE * count;

        if (sysarea_grows_up) {
            vaddr = sysarea_upper;
            sysarea_upper += size;
        } else {
            sysarea_lower -= size;
            vaddr = sysarea_lower;
        }

        //kprintf("pre_valloc @ %lx -> %lx\n", vaddr, paddr);

        int mapped = hal_map_range(vaddr, paddr, size, cache);
        panic_if(mapped != count, "Failed to map pages to HAL");

        return vaddr;
    }

    return 0;
}

void init_pre_palloc()
{
    struct loader_args *largs = get_loader_args();

    sysarea_grows_up = largs->sysarea_grows_up;
    sysarea_lower = align_down_vaddr(largs->sysarea_lower, PAGE_SIZE);
    sysarea_upper = align_up_vaddr(largs->sysarea_upper, PAGE_SIZE);
}
