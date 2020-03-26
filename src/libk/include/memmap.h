#ifndef __LIBK_INCLUDE_MEMMP_H__
#define __LIBK_INCLUDE_MEMMP_H__


#include "common/include/inttypes.h"
#include "common/include/memmap.h"


extern void claim_memmap_region(u64 start, u64 size, int flags);
extern u64 find_free_memmap_region(ulong size, ulong align);

extern void print_memmap();
extern struct loader_memmap_entry *get_memmap(int *num_entries, int *limit);
extern void init_libk_memmap(struct loader_memmap_entry *mmap, int num_entries, int limit);


#endif
