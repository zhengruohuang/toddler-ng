#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "common/include/page.h"
#include "common/include/msr.h"
#include "hal/include/setup.h"
#include "hal/include/lib.h"
#include "hal/include/int.h"
#include "hal/include/mem.h"
#include "hal/include/mp.h"


static void switch_page_table(struct l1table *page_table)
{
    //kprintf("To switch page table @ %lx\n", page_table);

    write_trans_tab_base0(page_table);
    inv_tlb_all();
    //TODO: atomic_membar();
}


void switch_to(ulong thread_id, struct reg_context *context,
               void *page_table, int user_mode, ulong asid, ulong tcb)
{
    //kprintf("cur_stack_top: %p, PC @ %p, SP @ %p, R0: %p\n", *cur_stack_top, context->pc, context->sp, context->r0);

    // Set up fast TCB access
    write_software_thread_id(tcb);

    // Mark interrupt state as enabled
    set_local_int_state(1);

    // Switch page dir
    switch_page_table(page_table);

    // Restore GPRs
    restore_context_gpr(*context);
}


void kernel_pre_dispatch(ulong thread_id, struct kernel_dispatch *kdi)
{
    struct loader_args *largs = get_loader_args();
    struct l1table *kernel_page_table = largs->page_table;
    switch_page_table(kernel_page_table);
}

void kernel_post_dispatch(ulong thread_id, struct kernel_dispatch *kdi)
{
    struct l1table *user_page_table = get_cur_running_page_table();
    switch_page_table(user_page_table);
}
