#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "common/include/syscall.h"
#include "hal/include/kprintf.h"
#include "hal/include/kernel.h"
#include "hal/include/hal.h"
#include "hal/include/lib.h"
#include "hal/include/mem.h"
#include "hal/include/int.h"
#include "hal/include/dev.h"
#include "hal/include/mp.h"


/*
 * Default handlers
 */
static int int_handler_dummy(struct int_context *context, struct kernel_dispatch *kdi)
{
//     kprintf("Interrupt, Vector: %lx, PC: %x, SP: %x, CPSR: %x\n",
//             context->vector, context->context->pc, context->context->sp, context->context->cpsr);

    panic("Unregistered interrupt @ %p", (void *)(ulong)context->vector);
    return INT_HANDLE_SIMPLE;
}

static int int_handler_dev(struct int_context *context, struct kernel_dispatch *kdi)
{
    return handle_dev_int(context, kdi);
}

static int int_handler_page_fault(struct int_context *ictxt, struct kernel_dispatch *kdi)
{
    kdi->num = SYSCALL_FAULT_PAGE;
    kdi->param0 = ictxt->error_code;

    return INT_HANDLE_CALL_KERNEL;
}


/*
 * Generic handler entry
 */
void int_handler(int seq, struct int_context *ictxt)
{
    //kprintf("Interrupt, seq: %d\n", seq);
    //while (1);

    // Mark local interrupts as disabled
    disable_local_int();

    // Handle halt
    handle_halt();

    // Prepare the handling
    struct kernel_dispatch kdispatch = { };
    kdispatch.regs = ictxt->regs;
    kdispatch.num = SYSCALL_INTERRUPT;

    ictxt->mp_seq = get_cur_mp_seq();

    // Handle the interrupt
    int handle_type = invoke_int_handler(seq, ictxt, &kdispatch);

    // Go to kernel if needed
    // kernel will call sched and never go back to this func
    if (handle_type & INT_HANDLE_CALL_KERNEL) {
        // Tell HAL we are in kernel
        int prev_in_user_mode = get_cur_running_in_user_mode();
        set_cur_running_in_user_mode(0);

        // Go to kernel!
        ulong tid = get_cur_running_thread_id();
        kernel_dispatch(tid, &kdispatch);

        // Restore in-user-mode
        set_cur_running_in_user_mode(prev_in_user_mode);
    }

    // Will now return, arch HAL must implement restoring from interrupt
}


void init_int_handler()
{
    set_default_int_handler(int_handler_dummy);
    set_int_handler(INT_SEQ_DUMMY, int_handler_dummy);
    set_int_handler(INT_SEQ_DEV, int_handler_dev);
    set_int_handler(INT_SEQ_PAGE_FAULT, int_handler_page_fault);
}
