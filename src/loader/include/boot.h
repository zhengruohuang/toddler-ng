#ifndef __LOADER_INCLUDE_BOOTINFO_H__
#define __LOADER_INCLUDE_BOOTINFO_H__


#include "common/include/inttypes.h"


/*
 * Memory map
 */
enum memmap_entry_type {
    MEMMAP_NONE = 0,
    MEMMAP_USABLE,
    MEMMAP_LOADER,
    MEMMAP_DEDICATED,
    MEMMAP_USED,
    MEMMAP_INVALID,
};

struct memmap_entry {
    u64 start;
    u64 size;
    int flags;
} packed_struct;

extern void init_memmap();
extern const struct memmap_entry *get_memmap(int *num_entries);
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
