#include "common/include/inttypes.h"
#include "common/include/syscall.h"
#include "hal/include/hal.h"
#include "hal/include/int.h"


static int int_handler_syscall(struct int_context *ictxt, struct kernel_dispatch *kdi)
{
    ulong param0 = 0, param1 = 0, param2 = 0, return0 = 0, return1 = 0;
    ulong num = arch_get_syscall_params(ictxt->regs, &param0, &param1, &param2);

    int success = 0;
    int call_kernel = 0;

    // See if we can quickly handle this syscall in HAL
    switch (num) {
    case SYSCALL_NONE_HAL:
        success = 1;
        break;
    case SYSCALL_PING_HAL:
        return0 = param0 + 1;
        return1 = param1 + 1;
        success = 1;
        break;
    case SYSCALL_GET_TCB:
        return0 = *get_per_cpu(ulong, cur_tcb_vaddr);
        success = 1;
        break;
    default:
        break;
    }

    if (!success) {
        // Handle arch-specific syscalls
        call_kernel = arch_handle_syscall(num, param0, param1, param2, &return0, &return1);
    }

    if (call_kernel) {
        // Prepare to call kernel
        kdi->num = num;
        kdi->param0 = param0;
        kdi->param1 = param1;
        kdi->param2 = param2;
    } else {
        // Prepare return value
        arch_set_syscall_return(ictxt->regs, success, return0, return1);
    }

    return call_kernel ? INT_HANDLE_CALL_KERNEL : INT_HANDLE_SIMPLE;
}


void init_syscall()
{
    set_int_handler(INT_SEQ_SYSCALL, int_handler_syscall);
}
