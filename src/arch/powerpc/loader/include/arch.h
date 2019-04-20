#ifndef __ARCH_POWERPC_LOADER_INCLUDE_ARCH_H__
#define __ARCH_POWERPC_LOADER_INCLUDE_ARCH_H__


#include "common/include/inttypes.h"


struct arch_loader_args {
    void *pht;
    ulong pht_mask;
    ulong pht_size;
};


#endif

