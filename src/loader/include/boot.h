#ifndef __LOADER_INCLUDE_BOOTINFO_H__
#define __LOADER_INCLUDE_BOOTINFO_H__


#include "common/include/inttypes.h"
#include "loader/include/loader.h"


/*
 * Memory map
 */
extern void print_memmap();
extern void init_memmap();
extern struct loader_memmap_entry *get_memmap(int *num_entries, int *limit);
extern u64 get_memmap_range(u64 *start);
extern void *memmap_alloc_phys(ulong size, ulong align);


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
extern int page_map_virt_to_phys(void *vaddr, void *paddr, ulong size,
    int cache, int exec, int write);


/*
 * Exec
 */
extern void load_hal_and_kernel();


#endif
