#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "hal/include/hal.h"
#include "hal/include/lib.h"
#include "hal/include/kprintf.h"
#include "hal/include/mem.h"


static int pre_palloc_enabled = 0;

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
    panic_if(!pre_palloc_enabled, "pre_palloc disabled!\n");

    ulong size = count * PAGE_SIZE;
    ulong align = PAGE_SIZE;

    u64 paddr_u64 = find_free_memmap_region_for_palloc(size, align);
    paddr_t paddr = cast_u64_to_paddr(paddr_u64);
    ppfn_t ppfn = paddr_to_ppfn(paddr);

    kprintf("pre_palloc @ %llx\n", (u64)paddr);
    return ppfn;

    //ulong paddr = (ulong)find_free_memmap_region(count * PAGE_SIZE, PAGE_SIZE);
    //ulong pfn = ADDR_TO_PFN(paddr);

    //kprintf("pre_palloc @ %lx\n", paddr);

    //return pfn;
}

ulong pre_valloc(int count, paddr_t paddr, int cache)
{
    panic_if(!pre_palloc_enabled, "pre_valloc disabled!\n");

    struct hal_arch_funcs *arch_funcs = get_hal_arch_funcs();

    int use_direct_access = arch_funcs->has_direct_access;
    if (use_direct_access &&
        !cache && !arch_funcs->has_direct_access_uncached
    ) {
        use_direct_access = 0;
    }

    if (use_direct_access) {
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

        kprintf("pre_valloc @ %lx -> %lx\n", vaddr, paddr);

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

    pre_palloc_enabled = 1;

    kprintf("Sysarea @ %lx - %lx, grows %s\n", sysarea_lower, sysarea_upper,
            sysarea_grows_up ? "up" : "down");
}

void disable_pre_palloc()
{
    pre_palloc_enabled = 0;
}
