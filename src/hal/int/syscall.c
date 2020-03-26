#include "common/include/inttypes.h"
#include "hal/include/hal.h"
#include "hal/include/int.h"


#define SYSCALL_PING 0


static int int_handler_syscall(struct int_context *ictxt, struct kernel_dispatch_info *kdi)
{
    ulong param0 = 0, param1 = 0, param2 = 0;
    ulong num = arch_get_syscall_params(ictxt->regs, &param0, &param1, &param2);

    ulong return0 = 0;
    ulong return1 = 0;
    int succeed = 0;

    int call_kernel = 1;

    // See if we can quickly handle this syscall in HAL
    if (num == SYSCALL_PING) {
        return0 = param0 + 1;
        succeed = 1;
        call_kernel = 0;
    }

    // Handle arch-specific syscalls
    call_kernel = arch_handle_syscall(num, param0, param1, param2, &return0, &return1);

    // Prepare to call kernel
    if (call_kernel) {
        kdi->dispatch_type = KERNEL_DISPATCH_SYSCALL;
        kdi->syscall.num = num;
        kdi->syscall.param0 = param0;
        kdi->syscall.param1 = param1;
        kdi->syscall.param2 = param2;

//         kprintf("Syscall from user!\n");
    }

    // Prepare return value
    else {
        arch_set_syscall_return(ictxt->regs, succeed, return0, return1);
    }

    return call_kernel ? INT_HANDLE_TYPE_KERNEL : INT_HANDLE_TYPE_HAL;
}


void init_syscall()
{
    set_int_handler(INT_SEQ_SYSCALL, int_handler_syscall);
}
