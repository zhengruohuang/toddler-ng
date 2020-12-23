#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "common/include/msr.h"
#include "hal/include/vecnum.h"
#include "hal/include/kprintf.h"
#include "hal/include/hal.h"
#include "hal/include/lib.h"
#include "hal/include/mem.h"
#include "hal/include/int.h"
#include "hal/include/mp.h"


/*
 * General handler
 */
static void *kernel_page_table;

void int_handler_entry(int except, struct reg_context *regs)
{
    // Mark local interrupts as disabled
    disable_local_int();

    //kprintf("Interrupt!\n");
    //while (1);

    //kprintf("General exception!\n");
    //kprintf("Interrupt, Except: %d, PC: %x, SP: %x, CPSR: %x\n",
    //        except, regs->pc, regs->sp, regs->cpsr);

    // Figure out the real vector number
    int seq = 0;
    ulong error_code = except;

    switch (except) {
    // System call
    case INT_VECTOR_SVC:
        seq = INT_SEQ_SYSCALL;
        break;

    // Page fault
    case INT_VECTOR_FETCH:
        seq = INT_SEQ_PAGE_FAULT;
        read_instr_fault_addr(error_code);
        break;

    case INT_VECTOR_DATA:
        seq = INT_SEQ_PAGE_FAULT;
        read_data_fault_addr(error_code);
        break;

    // Interrupt
    case INT_VECTOR_IRQ:
        seq = INT_SEQ_DEV;
        break;

    // Local timer
    case INT_VECTOR_FIQ:
        seq = INT_SEQ_PANIC;
        break;

    // Illegal
    case INT_VECTOR_RESET:
    case INT_VECTOR_RESERVED:
        seq = INT_SEQ_PANIC;
        break;

    // Not implement yet
    case INT_VECTOR_UNDEFINED:
        seq = INT_SEQ_PANIC;
        break;

    // Unknown
    default:
        seq = INT_SEQ_PANIC;
        break;
    }

    panic_if(seq == INT_SEQ_PANIC,
             "Panic int seq! except: %d, PC @ %x, SP @ %x, R0: %x\n",
             except, regs->pc, regs->sp, regs->r0);

    // Go to the generic handler!
    struct int_context intc;
    intc.vector = seq;
    intc.error_code = error_code;
    intc.regs = regs;

    int_handler(seq, &intc);
}


/*
 * Initialization
 */
#define FIXED_EVT_VADDR 0xffff0000ul

extern void int_entry_wrapper_begin();
extern void int_entry_wrapper_end();

decl_per_cpu(ulong, cur_int_stack_top);

void init_int_entry_mp()
{
    // Enable high addr vector
    struct sys_ctrl_reg sys_ctrl;
    read_sys_ctrl(sys_ctrl.value);
    sys_ctrl.high_except_vec = 1;
    write_sys_ctrl(sys_ctrl.value);

    // Set up stack top for context saving
    ulong stack_top = get_my_cpu_stack_top_vaddr() - sizeof(struct reg_context);

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

void init_int_entry()
{
    struct loader_args *largs = get_loader_args();
    kernel_page_table = largs->page_table;

    // Map the fixed EVT @ 0xffff0000ul
    ppfn_t ppfn = pre_palloc(1);
    paddr_t paddr = ppfn_to_paddr(ppfn);
    hal_map_range(FIXED_EVT_VADDR, paddr, PAGE_SIZE, 1);

    // Copy the vectors to the target address
    void *vec_target = (void *)FIXED_EVT_VADDR;
    kprintf("Copy interrupt vectors @ %p -> %p\n",
        int_entry_wrapper_begin, vec_target);

    memcpy(vec_target, int_entry_wrapper_begin,
           (ulong)int_entry_wrapper_end - (ulong)int_entry_wrapper_begin);

    init_int_entry_mp();
}
