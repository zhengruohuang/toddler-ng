#ifndef __LOADER_INCLUDE_EXPORT_H__
#define __LOADER_INCLUDE_EXPORT_H__


#include "common/include/inttypes.h"


struct loader_args {
    // Arch-specific data
    void *arch_args;
    ulong arch_args_bytes;

    // Important data structures
    void *memmap;
    void *devtree;
    void *page_table;

    // Coreimg
    void *coreimg;

    // Loader stack
    ulong stack_limit;
    ulong stack_limit_mp;

    // MP
    ulong mp_entry;

    // HAL virtual layout
    void *hal_entry;
    ulong hal_start, hal_end;

    // Kernel virtual layout
    void *kernel_entry;
    ulong kernel_start, kernel_end;

    // Sys area, which consists of HAL and Kernel
    ulong sysarea_lower, sysarea_upper;
    int sysarea_grows_up;
};


#endif
