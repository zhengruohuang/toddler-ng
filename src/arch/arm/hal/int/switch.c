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


void switch_to(ulong sched_id, struct reg_context *context,
               ulong page_dir_pfn, int user_mode, ulong asid, ulong tcb)
{
    // Switch page dir
    struct l1table **pl1tab = get_per_cpu(struct l1table *, cur_page_dir);

    paddr_t page_dir_paddr = ppfn_to_paddr(page_dir_pfn);
    struct l1table *page_dir = cast_paddr_to_ptr(page_dir_paddr);
    *pl1tab = page_dir;

//     kprintf("To switch page dir PFN @ %lx\n", page_dir_pfn);

    write_trans_tab_base0(page_dir);
    inv_tlb_all();
    //TODO: atomic_membar();

    // Copy context to interrupt stack
    ulong *cur_stack_top = get_per_cpu(ulong, cur_int_stack_top);
    memcpy((void *)*cur_stack_top, context, sizeof(struct reg_context));

//     kprintf("SVC stack @ %lx\n", *cur_stack_top);

    // Finally mark interrupts as enabled
    set_local_int_state(1);

//     if (user_mode) {
//         stop(0xbbbb);
//     }

    // Restore GPRs
    // TODO: restore_context_gpr();
}
