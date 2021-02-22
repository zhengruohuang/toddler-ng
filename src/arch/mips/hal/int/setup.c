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
 * Save extra contexts
 */
static void save_extra_context(struct reg_context *regs)
{
    // Save PC and branch delay slot state
    read_cp0_epc(regs->pc);

    // Save branch slot state
    struct cp0_cause cause;
    read_cp0_cause(cause.value);
    regs->delay_slot = cause.bd;
}


/*
 * MIPS handler entry
 */
void int_handler_entry(int except, struct reg_context *regs)
{
    // Set local interrupt state to disabled
    disable_local_int();

    // Save extra context
    save_extra_context(regs);

    // Check who is causing this interrupt
    struct cp0_cause cause;
    read_cp0_cause(cause.value);

    struct cp0_status sr;
    read_cp0_status(sr.value);

    // Get the bad address
    ulong bad_addr = 0;
    read_cp0_bad_vaddr(bad_addr);

    int seq = 0;
    ulong error_code = bad_addr;

//     if (except == 4 && cause.exc_code == 0)
//     kprintf("General exception! Except: %d, TI: %d, IP: %d, ECode: %d"
//             ", EXL: %d, ERL: %d, KSU: %d, PC: %lx, SP: %lx, Bad @ %lx\n",
//             except, cause.ti, cause.ip, cause.exc_code,
//             sr.exl, sr.erl, sr.ksu,
//             regs->pc, regs->sp, bad_addr);

    switch (except) {
    case EXCEPT_NUM_TLB_REFILL:
    case EXCEPT_NUM_TLB64_REFILL: {
        struct page_frame *page_table = get_cur_running_page_table();
        paddr_t paddr = translate(page_table, bad_addr);
        seq = paddr ? MIPS_INT_SEQ_TLB_REFILL : INT_SEQ_PAGE_FAULT;
        break;
    }
    case EXCEPT_NUM_CACHE_ERROR:
        seq = MIPS_INT_SEQ_CACHE_ERR;
        break;
    case EXCEPT_NUM_GENERAL:
        switch (cause.exc_code) {
        case 0:
            seq = INT_SEQ_DEV;
            break;
        case MIPS_INT_SEQ_TLB_REFILL:
        case MIPS_INT_SEQ_TLB_MISS_READ:
        case MIPS_INT_SEQ_TLB_MISS_WRITE: {
            // TODO: make sure TLB of bad_vaddr is half-mapped,
            // meaning the other ppfn slot out of the two is mapped,
            // but not the one for bad_vaddr
            struct page_frame *page_table = get_cur_running_page_table();
            paddr_t paddr = translate(page_table, bad_addr);
            seq = paddr ? MIPS_INT_SEQ_TLB_REFILL : INT_SEQ_PAGE_FAULT;
            break;
        }
        case MIPS_INT_SEQ_SYSCALL:
            regs->pc += 4;
            regs->delay_slot = 0;
            seq = INT_SEQ_SYSCALL;
            break;
        default:
            seq = cause.exc_code;
            break;
        }
        break;
    default:
        seq = INT_SEQ_PANIC;
        break;
    }

    // Go to the generic handler!
    struct int_context intc = { };
    intc.vector = seq;
    intc.error_code = error_code;
    intc.regs = regs;

    int_handler(seq, &intc);

    // Switch back previous program
    int user_mode = get_cur_running_in_user_mode();
    //kprintf("Return @ %lx, user: %d\n", regs->pc, user_mode);

    read_cp0_status(sr.value);
    sr.ksu = user_mode ? 0x2 : 0;
    sr.exl = 1;
    sr.erl = 0;
    write_cp0_status(sr.value);

    // Set EPC to PC
    write_cp0_epc(regs->pc);

    // Set CAUSE based on delay slot state
    read_cp0_cause(cause.value);
    cause.bd = regs->delay_slot ? 1 : 0;
    write_cp0_cause(cause.value);

    // TCB
    ulong tcb = get_cur_running_tcb();
    regs->k0 = tcb;
    regs->k1 = 0;

    // Set ASID in TLB EntryHi
    ulong asid = get_cur_running_asid();
    struct cp0_entry_hi hi;
    read_cp0_entry_hi(hi.value);
    hi.asid = asid;
    write_cp0_entry_hi(hi.value);

    // Finally enable local interrupts, since EXL is set, interrupts won't fire until eret
    enable_local_int();

    // Make sure k1 points to reg_context
    write_k1((ulong)regs);
}


/*
 * Initialization
 */
extern void raw_int_entry_base();

ulong *per_cpu_context_base_table = NULL;

decl_per_cpu(ulong, cur_int_stack_top);

void init_int_entry_mp()
{
    // Set BEV to 0 to enable custom exception handler location
    struct cp0_status sr;
    read_cp0_status(sr.value);
    sr.bev = 0;
    write_cp0_status(sr.value);

    // Set EBase
    struct cp0_ebase ebase;
    read_cp0_ebase(ebase.value);
#if (ARCH_WIDTH == 64)
    // Test write gate
    ebase.write_gate = 1;
    write_cp0_ebase(ebase.value);
    read_cp0_ebase(ebase.value);

    if (ebase.write_gate) {
        struct cp0_ebase64 ebase64;
        read_cp0_ebase64(ebase64.value);
        ebase64.base = ((ulong)&raw_int_entry_base) >> 12;
        write_cp0_ebase64(ebase64.value);
    } else {
        ulong wrapper_addr = (ulong)&raw_int_entry_base;
        assert((wrapper_addr >> 32) == 0xfffffffful);

        ebase.base = ((u32)wrapper_addr) >> 12;
        write_cp0_ebase(ebase.value);
    }
#else
    ebase.base = ((ulong)&raw_int_entry_base) >> 12;
    write_cp0_ebase(ebase.value);
#endif

    // Set up stack top for context saving
    ulong stack_top = get_my_cpu_stack_top_vaddr() - sizeof(struct reg_context);

    // Align the stack to 16B
    stack_top = ALIGN_DOWN(stack_top, 16);

    // Remember the stack top
    ulong *cur_stack_top = get_per_cpu(ulong, cur_int_stack_top);
    *cur_stack_top = stack_top;

    // Set up stack for each different mode
    kprintf("Set exception handler stack @ %lx\n", stack_top);

    // Set ctxt base table so that the raw handler knows where the stack is
    int cpu_id = ebase.cpunum;
    per_cpu_context_base_table[cpu_id] = stack_top;
}

void init_int_entry()
{
#if (ARCH_WIDTH == 32)
    ppfn_t ctxt_base_tab_ppfn = pre_palloc(1);
#else
    ppfn_t ctxt_base_tab_ppfn = pre_palloc(2);
#endif
    paddr_t ctxt_base_tab_paddr = ppfn_to_paddr(ctxt_base_tab_ppfn);
    per_cpu_context_base_table = cast_paddr_to_cached_seg(ctxt_base_tab_paddr);

    init_int_entry_mp();
}
