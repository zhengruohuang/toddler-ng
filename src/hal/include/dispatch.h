#ifndef __HAL_INCLUDE_DISPATCH__
#define __HAL_INCLUDE_DISPATCH__


#include "common/include/inttypes.h"
#include "common/include/context.h"


/*
 * Kernel dispatch info
 */
// enum kernel_dispatch_type {
//     KERNEL_DISPATCH_SYSCALL,
//     KERNEL_DISPATCH_INTERRUPT,
//     KERNEL_DISPATCH_EXCEPTION,
// };
//
// struct kernel_dispatch_info {
//     enum kernel_dispatch_type type;
//
//     struct reg_context *regs;
//     ulong tid;
//
//     union {
//         struct {
//             ulong num;
//
//             ulong param0;
//             ulong param1;
//             ulong param2;
//             ulong param3;
//
//             // Filled by kernel
//             ulong return0;
//             ulong return1;
//         } syscall;
//
//         struct {
//             ulong vector;
//             ulong param0;
//             ulong param1;
//         } interrupt;
//
//         struct {
//             ulong num;
//         } exception;
//     };
// };

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
