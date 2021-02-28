#ifndef __LOADER_INCLUDE_MEM_H__
#define __LOADER_INCLUDE_MEM_H__

#include "libk/include/mem.h"

/*
 * Memory map
 */
extern u64 get_memmap_range(u64 *start);
extern paddr_t memmap_alloc_phys(ulong size, ulong align);
extern paddr_t memmap_alloc_phys_page(int count);
extern void init_memmap();

/*
 * Paging
 */
extern void init_page();
extern int page_map_virt_to_phys(ulong vaddr, paddr_t paddr, ulong size,
                                 int cache, int exec, int write);

/*
 * Page table
 */
#include "common/include/abi.h"

#if (defined(ARCH_MIPS) || (defined(ARCH_OPENRISC)))
extern paddr_t translate_attri(void *page_table, ulong vaddr,
                               int *exec, int *read, int *write, int *cache);
extern paddr_t translate(void *page_table, ulong vaddr);
extern int map_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size,
                     int cache, int exec, int write, int kernel, int override);
extern int loader_map_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size,
                            int cache, int exec, int write);
#endif

#endif
