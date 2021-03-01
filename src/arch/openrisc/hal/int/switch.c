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
//     if (dev_int_fired)
//     kprintf("Switch to PC @ %lx, SP @ %lx, user: %d, ASID: %d, thread: %lx\n",
//             context->pc, context->sp, user_mode, asid, thread_id);

    // Disable MMU so that we get no TLB miss
    disable_mmu();

    // Switch page table
    set_page_table(page_table);

    // Set TCB
    context->tls = tcb;

    // Set interrupt state, the interrupt will be enabled by restoring SR
    set_local_int_state(1);

    // Restore registers
    restore_context_gpr(context);
}

void kernel_pre_dispatch(ulong thread_id, struct kernel_dispatch *kdi)
{
//     disable_mmu();
//     flush_tlb();
//
//     void *page_table = get_kernel_page_table();
//     set_page_table(page_table);
//
//     //enable_mmu();
}

void kernel_post_dispatch(ulong thread_id, struct kernel_dispatch *kdi)
{
//     disable_mmu();
//
//     struct running_context *rctxt = get_cur_running_context();
//     set_page_table(rctxt->page_table);
//
//     flush_tlb();
}
