#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "common/include/page.h"
#include "common/include/msr.h"
#include "hal/include/setup.h"
#include "hal/include/lib.h"
#include "hal/include/int.h"
#include "hal/include/mem.h"
#include "hal/include/mp.h"


void switch_to(ulong thread_id, struct reg_context *context,
               void *page_table, int user_mode, ulong asid, ulong tcb)
{
//     kprintf("Switch to PC @ %lx, SP @ %lx, user: %d, ASID: %d, thread: %lx\n",
//             context->pc, context->sp, user_mode, asid, thread_id);

    // Copy context to interrupt stack
    ulong *cur_stack_top = get_per_cpu(ulong, int_stack_top);
    struct reg_context *target_regs = (void *)*cur_stack_top;
    memcpy(target_regs, context, sizeof(struct reg_context));

    // Set up fast TCB access
    target_regs->tp = tcb;

    // Mark interrupt state as enabled
    set_local_int_state(1);

    // Switch page dir
    switch_page_table(page_table, asid);

    // Restore GPRs
    restore_context_gpr(target_regs);

    unreachable();
}


void kernel_pre_dispatch(ulong thread_id, struct kernel_dispatch *kdi)
{
    void *kernel_page_table = get_kernel_page_table();
    switch_page_table(kernel_page_table, 0);
}

void kernel_post_dispatch(ulong thread_id, struct kernel_dispatch *kdi)
{
    void *user_page_table = get_cur_running_page_table();
    ulong user_asid = get_cur_running_asid();
    switch_page_table(user_page_table, user_asid);
}
