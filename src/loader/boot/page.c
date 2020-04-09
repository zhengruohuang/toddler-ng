#include "common/include/inttypes.h"
#include "loader/include/loader.h"
#include "loader/include/lib.h"
#include "loader/include/mem.h"
#include "loader/include/boot.h"


static void *page_table = NULL;


static void map_memmap_1to1()
{
    // FIXME: need to figure out 1-to-1 mapping area
    u64 memstart = 0;
    u64 memsize = get_memmap_range(&memstart);

    paddr_t pmemstart = cast_u64_to_paddr(memstart);
    ulong vmemstart = cast_paddr_to_vaddr(pmemstart);
    ulong vmemsize = (ulong)memsize;    // FIXME: need a safe way

    page_map_virt_to_phys(vmemstart, pmemstart, vmemsize, 1, 1, 1);
}

void init_page()
{
    struct loader_arch_funcs *funcs = get_loader_arch_funcs();
    panic_if(!funcs->setup_page, "Arch loader must implement setup_page()!");

    page_table = funcs->setup_page();
    map_memmap_1to1();

    struct loader_args *largs = get_loader_args();
    largs->page_table = page_table;
}

int page_map_virt_to_phys(ulong vaddr, paddr_t paddr, ulong size,
    int cache, int exec, int write)
{
    //kprintf("To map %p -> %p, size: %lx\n", vaddr, paddr, size);

    struct loader_arch_funcs *funcs = get_loader_arch_funcs();
    if (funcs->map_range) {
        return funcs->map_range(page_table, vaddr, paddr, size, cache, exec, write);
    }

    return 0;
}
