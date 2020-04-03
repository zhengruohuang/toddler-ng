#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "hal/include/mp.h"
#include "hal/include/int.h"
#include "hal/include/lib.h"
#include "hal/include/hal.h"


decl_per_cpu(ulong, cur_running_sched_id);
decl_per_cpu(int, cur_in_user_mode);
decl_per_cpu(struct reg_context, cur_context);
decl_per_cpu(ulong, cur_tcb_vaddr);


void switch_context(ulong sched_id, struct reg_context *context,
                    ulong page_dir_pfn, int user_mode, ulong asid, ulong tcb)
{
//     kprintf("To switch context, R0: %x, PC: %x, SP: %x, CPSR: %x, ASID: %lx, user: %d, page dir: %lx, TCB @ %lx, sched: %lx\n",
//           context->r0, context->pc, context->sp, context->cpsr,
//           asid, user_mode, page_dir_pfn, tcb, sched_id);

    // Disable local interrupts
    disable_local_int();

    // Copy the context to local, thus prevent page fault upon switching page dir
    struct reg_context *per_cpu_context = get_per_cpu(struct reg_context, cur_context);
    memcpy(per_cpu_context, context, sizeof(struct reg_context));

    // Set sched id
    *get_per_cpu(ulong, cur_running_sched_id) = sched_id;
    *get_per_cpu(int, cur_in_user_mode) = user_mode;
    *get_per_cpu(ulong, cur_tcb_vaddr) = tcb;

    // Do the actual switch
    arch_switch_to(sched_id, per_cpu_context, page_dir_pfn, user_mode, asid, tcb);
}

void init_context()
{
    ulong *sched_id = get_per_cpu(ulong, cur_running_sched_id);
    *sched_id = 0;

    int *user_mode = get_per_cpu(int, cur_in_user_mode);
    *user_mode = 0;

    ulong *cur_tcb = get_per_cpu(ulong, cur_tcb_vaddr);
    *cur_tcb = 0;
}

void init_context_mp()
{
    init_context();
}
