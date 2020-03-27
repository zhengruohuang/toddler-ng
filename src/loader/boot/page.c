#include "common/include/inttypes.h"
#include "loader/include/loader.h"
#include "loader/include/lib.h"
#include "loader/include/boot.h"


static void *page_table = NULL;


static void map_memmap_1to1()
{
    u64 memstart = 0;
    u64 memsize = get_memmap_range(&memstart);

    // FIXME: u64 for paddr
    page_map_virt_to_phys((void *)memstart, (void *)memstart, memsize, 1, 1, 1);
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

int page_map_virt_to_phys(void *vaddr, void *paddr, ulong size,
    int cache, int exec, int write)
{
    //lprintf("To map %p -> %p, size: %lx\n", vaddr, paddr, size);

    struct loader_arch_funcs *funcs = get_loader_arch_funcs();
    if (funcs->map_range) {
        return funcs->map_range(page_table, vaddr, paddr, size, cache, exec, write);
    }

    return 0;
}
