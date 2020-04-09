#ifndef __LIBK_INCLUDE_MEMMP_H__
#define __LIBK_INCLUDE_MEMMP_H__


#include "common/include/inttypes.h"
#include "common/include/memmap.h"


enum memmap_alloc_match_type {
    MEMMAP_ALLOC_MATCH_IGNORE,      // Match everything
    MEMMAP_ALLOC_MATCH_EXACT,       // Match when tags == mask
    MEMMAP_ALLOC_MATCH_SET_ALL,     // Match when all 1s in mask are set in tags
    MEMMAP_ALLOC_MATCH_SET_ANY,     // Match when any 1  in mask is  set in tags
    MEMMAP_ALLOC_MATCH_UNSET_ALL,   // Match when all 1s in mask are not set in tags
    MEMMAP_ALLOC_MATCH_UNSET_ANY,   // Match when any 1  in mask is  not set
};


extern void claim_memmap_region(u64 start, u64 size, int flags);
extern void tag_memmap_region(u64 start, u64 size, u32 mask);
extern void untag_memmap_region(u64 start, u64 size, u32 mask);

extern u64 find_free_memmap_region(u64 size, u64 align, u32 mask, int match);
extern u64 find_free_memmap_region_for_palloc(u64 size, u64 align);

extern void print_memmap();
extern struct loader_memmap_entry *get_memmap(int *num_entries, int *limit);
extern void init_libk_memmap(struct loader_memmap_entry *mmap, int num_entries, int limit);


#endif
