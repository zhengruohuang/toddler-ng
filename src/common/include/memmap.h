#ifndef __COMMON_INCLUDE_MEMMAP_H__
#define __COMMON_INCLUDE_MEMMAP_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


enum loader_memmap_entry_type {
    MEMMAP_NONE = 0,
    MEMMAP_USABLE,
    MEMMAP_USED,
    MEMMAP_USED_RECLAIM,
    MEMMAP_INVALID,
};

struct loader_memmap_entry {
    u64 start;
    u64 size;
    int flags;
} packed_struct;


#endif
