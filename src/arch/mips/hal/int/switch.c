#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "common/include/page.h"
#include "common/include/msr.h"
#include "hal/include/setup.h"
#include "hal/include/lib.h"
#include "hal/include/int.h"
#include "hal/include/mem.h"
#include "hal/include/mp.h"


static decl_per_cpu(struct page_frame *, cur_page_dir);
static decl_per_cpu(ulong, cur_asid);


void switch_to(ulong thread_id, struct reg_context *context,
               void *page_table, int user_mode, ulong asid, ulong tcb)
{
    // Copy context to interrupt stack
    ulong *cur_stack_top = get_per_cpu(ulong, cur_int_stack_top);
    memcpy((void *)*cur_stack_top, context, sizeof(struct reg_context));

    // Switch page dir
    struct page_frame **my_page_table = get_per_cpu(struct page_frame *, cur_page_dir);
    *my_page_table = page_table;

    ulong *my_asid = get_per_cpu(ulong, cur_asid);
    *my_asid = asid;

    //kprintf("cur_stack_top: %p, PC @ %p, SP @ %p, R0: %p\n", *cur_stack_top, context->pc, context->sp, context->r0);
    kprintf("Switch to PC @ %lx, SP @ %lx\n", context->pc, context->sp);

    // Set up fast TCB access - k0
    struct reg_context *target_ctxt = (void *)*cur_stack_top;
    target_ctxt->k0 = tcb;

    // Set EPC to PC
    write_cp0_epc(target_ctxt->pc);

    // Set CAUSE based on delay slot state
    struct cp0_cause cause;
    read_cp0_cause(cause.value);
    cause.bd = target_ctxt->delay_slot ? 1 : 0;
    write_cp0_cause(cause.value);

    // Set SR based on user/kernel mode, also set EXL bit - switching is enabled
    struct cp0_status status;
    read_cp0_status(status.value);
    status.ksu = user_mode ? 0x2 : 0;
    status.exl = 1;
    status.erl = 0;
    write_cp0_status(status.value);

    // Set ASID in TLB EntryHi
    struct cp0_entry_hi hi;
    read_cp0_entry_hi(hi.value);
    hi.asid = asid;
    write_cp0_entry_hi(hi.value);

    // Finally enable local interrupts, since EXL is set, interrupts won't trigger until eret
    enable_local_int();

    // Restore GPRs
    restore_context_gpr(*cur_stack_top);
}

void init_switch_mp()
{
    struct page_frame **page_table = get_per_cpu(struct page_frame *, cur_page_dir);
    *page_table = NULL;

    ulong *asid = get_per_cpu(ulong, cur_asid);
    *asid = 0;
}

void init_switch()
{
    init_switch_mp();
}

void kernel_pre_dispatch(ulong thread_id, struct kernel_dispatch *kdi)
{
    // TODO: set TCB and ASID
//     struct loader_args *largs = get_loader_args();
//     struct l1table *kernel_page_table = largs->page_table;
//     switch_page_table(kernel_page_table);
}

void kernel_post_dispatch(ulong thread_id, struct kernel_dispatch *kdi)
{
//     struct l1table **pl1tab = get_per_cpu(struct l1table *, cur_page_dir);
//     struct l1table *user_page_table = *pl1tab;
//     switch_page_table(user_page_table);
}
