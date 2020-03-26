#ifndef __HAL_INCLUDE_DISPATCH__
#define __HAL_INCLUDE_DISPATCH__


#include "common/include/inttypes.h"
#include "common/include/context.h"


/*
 * Kernel dispatch info
 */
enum kernel_dispatch_type {
    KERNEL_DISPATCH_UNKNOWN,
    KERNEL_DISPATCH_SYSCALL,
    KERNEL_DISPATCH_INTERRUPT,
    KERNEL_DISPATCH_EXCEPTION,
    KERNEL_DISPATCH_PAGE_FAULT,
    KERNEL_DISPATCH_PROTECTION,
    KERNEL_DISPATCH_ILLEGAL_INSTR,
};

struct kernel_dispatch_info {
    // Filled by HAL
    enum kernel_dispatch_type dispatch_type;

    union {
        struct {
            // Filled by HAL
            ulong num;
            ulong param0;
            ulong param1;
            ulong param2;

            // Filled by kernel syscall handler
            ulong return0;
            ulong return1;
        } syscall;

        struct {
            // Filled by HAL
            ulong vector;
            ulong irq;

            ulong param0;
            ulong param1;
            ulong param2;
        } interrupt;
    };

    // Filled by HAL
    struct reg_context *regs;

    // Filled by kernel
    void *worker;
    void *proc;
    void *thread;
    void *sched;
};


#endif
