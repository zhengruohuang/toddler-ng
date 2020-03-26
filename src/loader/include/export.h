#ifndef __LOADER_INCLUDE_EXPORT_H__
#define __LOADER_INCLUDE_EXPORT_H__


#include "common/include/inttypes.h"


struct loader_args {
    // Arch-specific data
    void *arch_args;

    // Important data structures
    int num_memmap_entries, num_memmap_limit;
    void *memmap;
    void *devtree;
    void *page_table;

    // HAL virtual layout, hal_grow: >=0 = grow up, <0 = grow down
    void *hal_entry;
    ulong hal_start, hal_end;
    int hal_grow;

    // Kernel virtual layout
    void *kernel_entry;
    ulong kernel_start, kernel_end;
};


#endif
