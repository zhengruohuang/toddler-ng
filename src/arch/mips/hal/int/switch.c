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

    // Set EPC to PC
    write_cp0_epc(context->pc);

    // Set CAUSE based on delay slot state
    struct cp0_cause cause;
    read_cp0_cause(cause.value);
    cause.bd = context->delay_slot ? 1 : 0;
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
    restore_context_gpr(context);
}

void kernel_pre_dispatch(ulong thread_id, struct kernel_dispatch *kdi)
{
    // TODO: set TCB, ASID, and page table
}

void kernel_post_dispatch(ulong thread_id, struct kernel_dispatch *kdi)
{
}
