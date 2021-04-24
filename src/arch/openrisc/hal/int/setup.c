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
 * TLB miss stack
 */
// TLB miss stack is used when an TLB miss occurs, with MMU auto disabled
// Otherwise the default int stack is used, and MMU is re-enabled
// Note that a TLB miss may end up being a page fault,
// and in such case, stack is switched to default along with MMU enabled
#define TLB_STACK_SIZE PAGE_SIZE

static ulong tlb_miss_stack_area_base = 0;

static decl_per_cpu(ulong, tlb_miss_stack_top);
static decl_per_cpu(ulong, tlb_miss_stack_base);

static inline void check_tlb_miss_stack()
{
    ulong stack_base = *get_per_cpu(ulong, tlb_miss_stack_base);
    check_stack_magic(stack_base);
    check_stack_pos(stack_base);
}

static ulong init_tlb_miss_stack_mp()
{
    // Find out current CPU's stack base
    int mp_seq = arch_get_cur_mp_seq();
    ulong stack_base_vaddr = tlb_miss_stack_area_base + mp_seq * TLB_STACK_SIZE;

    // Set up and remember stack limit
    ulong *cur_quick_stack_base = get_per_cpu(ulong, tlb_miss_stack_base);
    *cur_quick_stack_base = stack_base_vaddr;
    set_stack_magic(stack_base_vaddr);

    // Set up stack top for context saving
    ulong stack_top = stack_base_vaddr + TLB_STACK_SIZE;
    stack_top -= 16 + sizeof(struct reg_context);

    // Align the stack to 16B
    stack_top = ALIGN_DOWN(stack_top, 16);

    // Remember stack top
    ulong *cur_tlb_stack_top = get_per_cpu(ulong, tlb_miss_stack_top);
    *cur_tlb_stack_top = stack_top;

    return stack_top;
}

static void init_tlb_miss_stack()
{
    int num_cpus = get_num_cpus();
    ulong tlb_stack_area_size = num_cpus * TLB_STACK_SIZE;
    tlb_stack_area_size = align_up_vsize(tlb_stack_area_size, PAGE_SIZE);

    int num_pages = tlb_stack_area_size / PAGE_SIZE;
    ppfn_t stack_base_ppfn = pre_palloc(num_pages);
    paddr_t stack_base_paddr = ppfn_to_paddr(stack_base_ppfn);

    tlb_miss_stack_area_base = pre_valloc(num_pages, stack_base_paddr, 1, 1);
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

    ulong epc = 0;
    read_epcr0(epc);

    ulong eear = 0;
    read_eear0(eear);

    for (int i = 0; i < 32; i++) {
        ulong r = 0;
        read_gpr1(r, i);

        if (r == 0x108aa2c) {
            kprintf("here?\n");
            while (1);
        }
    }

    int itlb = except == EXCEPT_NUM_ITLB_MISS ? 1 : 0;
    int err = tlb_refill(itlb, eear);
    check_tlb_miss_stack();

    if (!err) {
        restore_context_gpr_from_shadow1();
        unreachable();
    }

//     kprintf("Page fault @ %lx, PC @ %lx\n", eear, epc);
}

void int_handler_entry(int except, struct reg_context *regs)
{
//     kprintf_unlocked("kernel int handler entry, regs @ %p\n", regs);

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

//     if (except != EXCEPT_NUM_SYSCALL)
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

void init_int_entry_mp()
{
    // Set exception base addr
    struct except_base_reg evbar;
    evbar.value = 0;
    evbar.ppn = (ulong)&raw_int_entry_base >> PAGE_BITS;
    write_evbar(evbar.value);
    kprintf("EVBAR @ %x, entry @ %p\n", evbar.value, &raw_int_entry_base);

    // Set up TLB miss stack
    ulong tlb_stack_top = init_tlb_miss_stack_mp();

    // Set up kernel stack
    struct reg_context *int_ctxt = get_cur_int_reg_context();
    ulong kernel_stack_top = (ulong)(void *)int_ctxt;

    // Set up ctxt2 s0 (r14) and s1 (r16)
    write_gpr2(tlb_stack_top, 14);
    write_gpr2(kernel_stack_top, 16);

    kprintf("Stack top, quick @ %lx, kernel @ %lx\n",
            tlb_stack_top, kernel_stack_top);
}

void init_int_entry()
{
    init_tlb_miss_stack();
    init_int_entry_mp();
}
