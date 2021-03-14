#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "hal/include/mp.h"
#include "hal/include/int.h"
#include "hal/include/lib.h"
#include "hal/include/hal.h"


decl_per_cpu(struct reg_context *, cur_int_reg_context);
decl_per_cpu(struct running_context, cur_running_context);


void switch_context(ulong thread_id, struct reg_context *context,
                    void *page_table, int user_mode, ulong asid, ulong tcb)
{
//     kprintf("To switch context, thread_id: %lx, page_table @ %lx, user_mode: %d\n",
//             thread_id, page_table, user_mode);

    // Disable local interrupts
    disable_local_int();

    // Copy the context to local, thus prevent page fault upon switching page dir
    struct reg_context *per_cpu_ctxt = *get_per_cpu(struct reg_context *, cur_int_reg_context);
    memcpy(per_cpu_ctxt, context, sizeof(struct reg_context));

    // Save running ctxt
    struct running_context *rctxt = get_per_cpu(struct running_context, cur_running_context);
    rctxt->kernel_context = context;
    rctxt->thread_id = thread_id;
    rctxt->page_table = page_table;
    rctxt->user_mode = user_mode;
    rctxt->tcb = tcb;
    rctxt->asid = asid;

    // Do the actual switch
    arch_switch_to(thread_id, per_cpu_ctxt, page_table, user_mode, asid, tcb);
}

void init_context_mp()
{
    // Set interrupt context and stack top
    ulong stack_top = get_my_cpu_stack_top_vaddr() - sizeof(struct reg_context) - 16;
    stack_top = ALIGN_DOWN(stack_top, 16);

    struct reg_context **ctxt = get_per_cpu(struct reg_context *, cur_int_reg_context);
    *ctxt = (void *)stack_top;

    // Set current running context
    struct running_context *rctxt = get_per_cpu(struct running_context, cur_running_context);
    memzero(rctxt, sizeof(struct running_context));

    // Set current kernel table to kernel's
    rctxt->page_table = get_loader_args()->page_table;
}

void init_context()
{
    init_context_mp();
}
