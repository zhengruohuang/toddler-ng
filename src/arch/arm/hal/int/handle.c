#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "common/include/msr.h"
#include "hal/include/vecnum.h"
#include "hal/include/kprintf.h"
#include "hal/include/hal.h"
#include "hal/include/lib.h"
#include "hal/include/int.h"
#include "hal/include/mp.h"


/*
 * Asm externs
 */
extern void int_entry_wrapper_begin();
extern void int_entry_wrapper_end();


/*
 * General handler
 */
static void *kernel_page_table;

void int_handler_entry(int except, struct reg_context *context)
{
    // Switch to kernel's page table
    write_trans_tab_base0(kernel_page_table);
    inv_tlb_all();
    // FIXME: atomic_membar();

    // Mark local interrupts as disabled
    disable_local_int();

//     kprintf("General exception!\n");
//     kprintf("Interrupt, Except: %d, PC: %x, SP: %x, CPSR: %x\n",
//             except, context->pc, context->sp, context->cpsr);

    // Figure out the real vector number
    int vector = 0;

    switch (except) {
    // System call
    case INT_VECTOR_SVC:
        vector = INT_VECTOR_SYSCALL;
        break;

    // Page fault
    case INT_VECTOR_FETCH:
    case INT_VECTOR_DATA:
        break;

    // Interrupt
    case INT_VECTOR_IRQ:
        vector = 0; // TODO: periph_get_irq_vector();
        break;

    // Local timer
    case INT_VECTOR_FIQ:
        vector = 0; // TODO: periph_get_fiq_vector();
        break;

    // Illegal
    case INT_VECTOR_RESET:
    case INT_VECTOR_RESERVED:
        vector = INT_VECTOR_DUMMY;
        break;

    // Not implement yet
    case INT_VECTOR_UNDEFINED:
        vector = INT_VECTOR_DUMMY;
        break;

    // Unknown
    default:
        vector = INT_VECTOR_DUMMY;
        break;
    }

    // Get the actual interrupt handler
    int_handler_t handler = get_int_handler(vector);

    // Call the real handler, the return value indicates if we need to call kernel
    struct int_context intc;
    intc.vector = vector;
    intc.error_code = except;
    intc.regs = context;

    struct kernel_dispatch_info kdispatch;
    kdispatch.regs = context;
    kdispatch.dispatch_type = KERNEL_DISPATCH_UNKNOWN;
    kdispatch.syscall.num = 0;

    // Call the handler
    int handle_type = handler(&intc, &kdispatch);

    if (except == INT_VECTOR_FIQ) {
        // FIQ handler must have TAKE_OVER or HAL type
        if (handle_type != INT_HANDLE_TYPE_HAL && handle_type != INT_HANDLE_TYPE_TAKEOVER) {
            panic("FIQ handler must have TAKE_OVER or HAL type!\n");
        }

        // Switch to kernel if FIQ did not preempt any normal interrupts
        struct proc_status_reg status;
        status.value = context->cpsr;

        if (status.mode == 0x10 || status.mode == 0x1f) {
            handle_type = INT_HANDLE_TYPE_KERNEL;
        }
    }

    // Note that if kernel is invoked,
    // kernel will call sched and never go back to this int handler
    if (handle_type == INT_HANDLE_TYPE_KERNEL) {
        // Tell HAL we are in kernel
        *get_per_cpu(int, cur_in_user_mode) = 0;

        // Go to kernel!
        //kernel_dispatch(&kdispatch);
    }

    while (1);
//     panic("Need to implement lazy scheduling!");
}


/*
 * Initialization
 */
decl_per_cpu(ulong, cur_int_stack_top);

void init_handler_secondary()
{
    // Enable high addr vector
    struct sys_ctrl_reg sys_ctrl;
    read_sys_ctrl(sys_ctrl.value);
    sys_ctrl.high_except_vec = 1;
    write_sys_ctrl(sys_ctrl.value);

    // Set up stack top for context saving
    ulong stack_top = get_my_cpu_area_start_vaddr() +
        PER_CPU_STACK_TOP_OFFSET - sizeof(struct reg_context);

    // Align the stack to 16B
    stack_top = ALIGN_DOWN(stack_top, 16);

    // Remember the stack top
    ulong *cur_stack_top = get_per_cpu(ulong, cur_int_stack_top);
    *cur_stack_top = stack_top;

    // Set up stack for each different mode
    kprintf("Set exception handler stack @ %lx\n", stack_top);

    ulong saved_sp, saved_lr;
    __asm__ __volatile__ (
        // Save current sp and lr
        "mov %[saved_sp], sp;"
        "mov %[saved_lr], lr;"

        // SVC
        "cpsid aif, #0x13;"
        "mov sp, %[stack];"

        // UNDEF
        "cpsid aif, #0x1b;"
        "mov sp, %[stack];"

        // ABT
        "cpsid aif, #0x17;"
        "mov sp, %[stack];"

        // IRQ
        "cpsid aif, #0x12;"
        "mov sp, %[stack];"

        // FIQ
        "cpsid aif, #0x11;"
        "mov sp, %[stack];"

        // Move to System mode
        "cpsid aif, #0x1f;"

        // Restore saved sp and lr
        "mov sp, %[saved_sp];"
        "mov lr, %[saved_lr];"

        : [saved_sp] "=&r" (saved_sp), [saved_lr] "=&r" (saved_lr)
        : [stack] "r" (stack_top)
    );
}

void init_handler()
{
    struct loader_args *largs = get_loader_args();
    kernel_page_table = largs->page_table;

    // Copy the vectors to the target address
    void *vec_target = (void *)0xffff0000ul;
    kprintf("Copy interrupt vectors @ %p -> %p\n",
        int_entry_wrapper_begin, vec_target);

    memcpy(vec_target, int_entry_wrapper_begin,
           (ulong)int_entry_wrapper_end - (ulong)int_entry_wrapper_begin);

    init_handler_secondary();
}
