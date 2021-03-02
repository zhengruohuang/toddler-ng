#ifndef __LOADER_INCLUDE_MEM_H__
#define __LOADER_INCLUDE_MEM_H__

#include "libk/include/mem.h"

/*
 * Memory map
 */
extern u64 get_memmap_range(u64 *start);
extern paddr_t memmap_alloc_phys(ulong size, ulong align);
extern ppfn_t memmap_palloc(int count);
extern void init_memmap();

/*
 * Paging
 */
extern void init_page();
extern int page_map_virt_to_phys(ulong vaddr, paddr_t paddr, ulong size,
                                 int cache, int exec, int write);


#endif
