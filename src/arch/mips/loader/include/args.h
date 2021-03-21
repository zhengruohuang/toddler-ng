#ifndef __ARCH_MIPS_LOADER_INCLUDE_ARGS_H__
#define __ARCH_MIPS_LOADER_INCLUDE_ARGS_H__

#include "common/include/inttypes.h"

/*
 * MIPS-specific loader args
 */
struct mips_loader_args {
    struct {
        u8 *bool_array;
        int num_entries;
    } cpunum_table;
};

#endif
