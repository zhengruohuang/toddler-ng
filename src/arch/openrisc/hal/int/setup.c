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
#include "hal/include/setup.h"


/*
 * Exception names
 */
static char *except_names[] = {
    "None",
    "Reset",
    "Bus Error",
    "Data Page Fault",
    "Instr Page Fault",
    "Tick Timer",
    "Alignment",
    "Illegal Instr",
    "External Interrupt",
    "DTLB Miss",
    "ITLB Miss",
    "Range",
    "Syscall",
    "Floating Point",
    "Trap",
    "Reserved"
};

static inline char *get_except_name(int except)
{
    char *ename = "Unknown";
    if (except < sizeof(except_names) / sizeof(char *)) {
        ename = except_names[except];
    }
    return ename;
}


/*
 * Save registers to memory
 */
static inline void save_ctxt1_regs_to_mem(struct reg_context *regs)
{
    for (int i = 0; i < 32; i++) {
        read_gpr1(regs->regs[i], i);
    }

    read_epcr0(regs->pc);
    read_esr0(regs->sr);

    struct supervision_reg sr;
    read_sr(sr.value);
    regs->delay_slot = sr.delay_slot_except ? 1 : 0;
}


/*
 * OpenRISC handler entry
 */
void tlb_miss_handler_entry(int except, struct reg_context *regs)
{
    panic_if(except != EXCEPT_NUM_ITLB_MISS && except != EXCEPT_NUM_DTLB_MISS,
             "Must be TLB miss!\n");

    //kprintf("TLB int handler entry @ %p, %p\n", quick_regs, kernel_regs);

    ulong epc = 0;
    read_epcr0(epc);

    ulong eear = 0;
    read_eear0(eear);

    int itlb = except == EXCEPT_NUM_ITLB_MISS ? 1 : 0;
    int err = tlb_refill(itlb, eear);
    if (!err) {
        restore_context_gpr_from_shadow1();
        unreachable();
    }
}

void int_handler_entry(int except, struct reg_context *regs)
{
    //kprintf("kernel int handler entry @ %p, %p\n",  kernel_regs, quick_regs);

    disable_mmu();
    disable_local_int();

    // Save registers to memory
    save_ctxt1_regs_to_mem(regs);

    ulong epc = 0;
    read_epcr0(epc);

    ulong eear = 0;
    read_eear0(eear);

    // Exception handling
    int seq = 0;
    ulong error_code = eear;

//     if (except != EXCEPT_NUM_SYSCALL && except != EXCEPT_NUM_TIMER)
//     kprintf("Exception #%d: %s, EPC @ %lx, EEAR @ %lx\n",
//             except, get_except_name(except), epc, eear);

    switch (except) {
    case EXCEPT_NUM_DATA_PAGE_FAULT:
    case EXCEPT_NUM_INSTR_PAGE_FAULT:
        seq = INT_SEQ_PAGE_FAULT;
        break;
    case EXCEPT_NUM_SYSCALL:
        seq = INT_SEQ_SYSCALL;
        break;
    case EXCEPT_NUM_TIMER:
    case EXCEPT_NUM_INTERRUPT:
        seq = INT_SEQ_DEV;
        error_code = except;
        break;
    default:
        seq = INT_SEQ_PANIC;
        panic("Unhandled exception #%d: %s, EPC @ %lx, EEAR @ %lx\n",
              except, get_except_name(except), epc, eear);
        break;
    }

    // TODO: add ns16550 hal driver and move following to kernel_dispatch
    disable_mmu();
    flush_tlb();
    void *page_table = get_kernel_page_table();
    set_page_table(page_table);
    enable_mmu();

    // Go to the generic handler!
    struct int_context intc = { };
    intc.vector = seq;
    intc.error_code = error_code;
    intc.regs = regs;

    int_handler(seq, &intc);

    // TODO
    disable_mmu();
    struct running_context *rctxt = get_cur_running_context();
    set_page_table(rctxt->page_table);
    flush_tlb();

    // Return to the context prior to exception
    set_local_int_state(1);
    restore_context_gpr(regs);

    unreachable();
}


/*
 * Init
 */
extern void raw_int_entry_base();

// TLB miss stack is used when an TLB miss occurs, with MMU auto disabled
// Otherwise the default int stack is used, and MMU is re-enabled
// Note that a TLB miss may end up being a page fault,
// and in such case, stack is switched to default along with MMU enabled
static decl_per_cpu(ulong, tlb_miss_stack_top);

static inline ulong _setup_default_int_stack()
{
    struct reg_context *int_ctxt = get_cur_int_reg_context();
    return (ulong)(void *)int_ctxt;
}

static inline ulong _setup_tlb_miss_stack()
{
    ppfn_t stack_base_ppfn = pre_palloc(1);
    paddr_t stack_base_paddr = ppfn_to_paddr(stack_base_ppfn);
    ulong stack_base_vaddr = pre_valloc(1, stack_base_paddr, 1);

    // Set up stack top for context saving
    ulong stack_top = stack_base_vaddr + PAGE_SIZE - 32 - sizeof(struct reg_context);

    // Align the stack to 16B
    stack_top = ALIGN_DOWN(stack_top, 16);

    // Remember the stack top
    ulong *cur_quick_stack_top = get_per_cpu(ulong, tlb_miss_stack_top);
    *cur_quick_stack_top = stack_top;

    return stack_top;
}

void init_int_entry_mp()
{
    // Set exception base addr
    struct except_base_reg evbar;
    evbar.value = 0;
    evbar.ppn = (ulong)&raw_int_entry_base >> PAGE_BITS;
    write_evbar(evbar.value);
    kprintf("EVBAR @ %x, entry @ %p\n", evbar.value, &raw_int_entry_base);

    // Set up stacks
    ulong kernel_stack_top = _setup_default_int_stack();
    ulong quick_stack_top = _setup_tlb_miss_stack();

    kprintf("Stack top, quick @ %lx, kernel @ %lx\n", quick_stack_top, kernel_stack_top);

    // Set up ctxt2 s0 (r14) and s1 (r16)
    write_gpr2(quick_stack_top, 14);
    write_gpr2(kernel_stack_top, 16);
}

void init_int_entry()
{
    init_int_entry_mp();
}
