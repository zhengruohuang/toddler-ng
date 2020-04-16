#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "common/include/msr.h"
#include "hal/include/kprintf.h"
#include "hal/include/hal.h"
#include "hal/include/lib.h"
#include "hal/include/mem.h"
#include "hal/include/int.h"
#include "hal/include/dev.h"
#include "hal/include/mp.h"


/*
 * Default handlers
 */
static int int_handler_dummy(struct int_context *context, struct kernel_dispatch_info *kdi)
{
//     kprintf("Interrupt, Vector: %lx, PC: %x, SP: %x, CPSR: %x\n",
//             context->vector, context->context->pc, context->context->sp, context->context->cpsr);

    panic("Unregistered interrupt @ %p", (void *)(ulong)context->vector);
    return INT_HANDLE_SIMPLE;
}

static int int_handler_dev(struct int_context *context, struct kernel_dispatch_info *kdi)
{
    return handle_dev_int(context, kdi);
}



void int_handler(int seq, struct int_context *ictxt)
{
    kprintf("Interrupt, seq: %d\n", seq);
    //while (1);

    // Mark local interrupts as disabled
    disable_local_int();

    // Handle halt
    handle_halt();

    // Prepare the handling
    struct kernel_dispatch_info kdispatch;
    kdispatch.regs = ictxt->regs;
    kdispatch.dispatch_type = KERNEL_DISPATCH_UNKNOWN;
    kdispatch.syscall.num = 0;

    ictxt->mp_seq = get_cur_mp_seq();

    // Handle the interrupt
    int handle_type = invoke_int_handler(seq, ictxt, &kdispatch);

    // Go to kernel if needed
    // kernel will call sched and never go back to this func
    if (handle_type & INT_HANDLE_CALL_KERNEL) {
        // Tell HAL we are in kernel
        //*get_per_cpu(int, cur_in_user_mode) = 0;

        // Go to kernel!
        //kernel_dispatch(&kdispatch);
    }

    //while (1);
//     panic("Need to implement lazy scheduling!");
}


void init_int_handler()
{
    set_int_handler(INT_SEQ_DUMMY, int_handler_dummy);
    set_int_handler(INT_SEQ_DEV, int_handler_dev);
}
