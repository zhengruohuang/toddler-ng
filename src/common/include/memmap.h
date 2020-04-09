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

enum loader_memmap_entry_tag {
    MEMMAP_TAG_NONE             = 0,
    MEMMAP_TAG_NORMAL           = 0x1,  // Safe for general-purpose allocation
    MEMMAP_TAG_DIRECT_MAPPED    = 0x2,  // Direct mapped in kernel
    MEMMAP_TAG_DIRECT_ACCESS    = 0x4,  // Direct accessible, only for MIPS and Alpha
    MEMMAP_TAG_LEGACY_DMA       = 0x8,  // accessible by legacy DMA devices
};

struct loader_memmap_entry {
    u64 start;
    u64 size;
    int flags;
    u32 tags;
};


#endif
