#ifndef __HAL_INCLUDE_DISPATCH__
#define __HAL_INCLUDE_DISPATCH__


#include "common/include/inttypes.h"
#include "common/include/context.h"


/*
 * Kernel dispatch info
 */
struct kernel_dispatch {
    ulong num;
    ulong tid;
    struct reg_context *regs;

    ulong param0;
    ulong param1;
    ulong param2;
    ulong param3;

    union {
        struct {
            // Filled by kernel
            ulong return0;
            ulong return1;
        };

        struct {
            ulong vector;
        };
    };
};


#endif
