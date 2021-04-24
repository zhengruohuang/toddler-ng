#include "common/include/inttypes.h"
#include "common/include/syscall.h"
#include "hal/include/hal.h"
#include "hal/include/int.h"


static int int_handler_syscall(struct int_context *ictxt, struct kernel_dispatch *kdi)
{
    ulong param0 = 0, param1 = 0, param2 = 0, return0 = 0, return1 = 0;
    ulong num = arch_get_syscall_params(ictxt->regs, &param0, &param1, &param2);

    int handled = 0;

    // See if we can quickly handle this syscall in HAL
    switch (num) {
    case SYSCALL_HAL_NONE:
        handled = 1;
        break;
    case SYSCALL_HAL_PING:
        return0 = param0 + 1;
        return1 = param1 + 1;
        handled = 1;
        break;
    case SYSCALL_HAL_GET_TIB:
        return0 = get_cur_running_tcb();
        handled = 1;
        break;
    case SYSCALL_HAL_IOPORT:
        if (arch_hal_has_io_port()) {
            ulong port = param0;
            ulong size = param1 & 0xff;
            ulong write = param1 >> 8;
            ulong value = param2;
            if (write) {
                arch_hal_ioport_write(port, size, value);
            } else {
                return0 = arch_hal_ioport_read(port, size);
            }
            handled = 1;
        }
        break;
    default:
        break;
    }

    if (!handled) {
        // Handle arch-specific syscalls
        handled = arch_handle_syscall(num, param0, param1, param2, 0, &return0, &return1);
    }

    if (!handled) {
        // Prepare to call kernel
        kdi->num = num;
        kdi->param0 = param0;
        kdi->param1 = param1;
        kdi->param2 = param2;
    } else {
        // Prepare return value
        arch_set_syscall_return(ictxt->regs, 0, return0, return1);
    }

    return !handled ? INT_HANDLE_CALL_KERNEL : INT_HANDLE_SIMPLE;
}


void init_syscall()
{
    set_int_handler(INT_SEQ_SYSCALL, int_handler_syscall);
}
