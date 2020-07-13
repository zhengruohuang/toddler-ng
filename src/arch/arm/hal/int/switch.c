#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "common/include/mem.h"
#include "common/include/msr.h"
#include "hal/include/setup.h"
#include "hal/include/lib.h"
#include "hal/include/int.h"
#include "hal/include/mem.h"
#include "hal/include/mp.h"


decl_per_cpu(struct l1table *, cur_page_dir);


void switch_to(ulong thread_id, struct reg_context *context,
               void *page_table, int user_mode, ulong asid, ulong tcb)
{
    // Copy context to interrupt stack
    ulong *cur_stack_top = get_per_cpu(ulong, cur_int_stack_top);
    memcpy((void *)*cur_stack_top, context, sizeof(struct reg_context));

    // Mark interrupt state as enabled
    set_local_int_state(1);

    // Switch page dir
    struct l1table **pl1tab = get_per_cpu(struct l1table *, cur_page_dir);
    *pl1tab = page_table;

    //kprintf("To switch page table @ %lx\n", page_table);
    write_trans_tab_base0(page_table);
    inv_tlb_all();
    //TODO: atomic_membar();

    //kprintf("cur_stack_top: %p, PC @ %p, SP @ %p, R0: %p\n", *cur_stack_top, context->pc, context->sp, context->r0);
    //kprintf("Target PC @ %p\n", *(ulong *)*cur_stack_top);

    // Restore GPRs
    restore_context_gpr(*cur_stack_top);
}
