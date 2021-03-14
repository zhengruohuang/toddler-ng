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

    // Set up fast TCB access
    context->tp = tcb;

    // Mark interrupt state as enabled
    set_local_int_state(1);

    // Switch page dir
    switch_page_table(page_table, asid);

    // Restore GPRs
    restore_context_gpr(context);

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
