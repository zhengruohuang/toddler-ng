#ifndef __LOADER_INCLUDE_BOOTINFO_H__
#define __LOADER_INCLUDE_BOOTINFO_H__


#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "loader/include/loader.h"
#include "libk/include/memmap.h"


/*
 * Memory map
 */
extern u64 get_memmap_range(u64 *start);
extern paddr_t memmap_alloc_phys(ulong size, ulong align);
extern void init_memmap();


/*
 * Coreimg
 */
extern void *find_supplied_coreimg(int *size);
extern void init_coreimg();
extern void *coreimg_find_file(const char *name);


/*
 * Paging
 */
extern void init_page();
extern int page_map_virt_to_phys(ulong vaddr, paddr_t paddr, ulong size,
    int cache, int exec, int write);


/*
 * Exec
 */
extern void load_hal_and_kernel();


#endif
