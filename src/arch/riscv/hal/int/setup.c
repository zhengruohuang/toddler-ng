#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "common/include/msr.h"
#include "hal/include/vecnum.h"
#include "hal/include/setup.h"
#include "hal/include/kprintf.h"
#include "hal/include/hal.h"
#include "hal/include/mem.h"
#include "hal/include/int.h"
#include "hal/include/mp.h"


/*
 * General handler
 */
#define ECALL_ENCODING 0x00000073

static inline void save_extra_context(struct reg_context *regs)
{
    read_sepc(regs->pc);
    read_sstatus(regs->status);
}

static inline u32 read_instr4(ulong pc)
{
    u16 lower = *(u16 *)(void *)(pc + 0);
    u16 upper = *(u16 *)(void *)(pc + 0x2ul);
    u32 instr = ((u32)upper << 16) | (u32)lower;
    return instr;
}

static inline int is_kernel_ecall_hack(ulong pc)
{
    // PC must be aligned to 8-byte
    if (pc & 0x7ul) {
        return 0;
    }

    // Must be 4-byte zero
    if (read_instr4(pc)) {
        return 0;
    }

    // Next instr must be ecall
    if (pc < 4 && read_instr4(pc + 4) != ECALL_ENCODING) {
        return 0;
    }

    return 1;
}

void int_handler_entry(struct reg_context *regs)
{
    save_extra_context(regs);

    // Mark local interrupts as disabled
    disable_local_int();

    struct cause_reg cause;
    read_scause(cause.value);

//     kprintf("Interrupt: %d, cause: %u, regs @ %p, PC @ %lx, a0: %lx\n",
//             cause.interrupt, cause.except_code, regs, regs->pc, regs->a0);

    // Figure out seq number
    int seq = 0;
    ulong error_code = 0;

    if (cause.interrupt) {
        seq = INT_SEQ_DEV;
        error_code = cause.except_code;
    } else {
        switch (cause.except_code) {
        case EXCEPT_CODE_INSTR_ILLEGAL:
            if (is_kernel_ecall_hack(regs->pc)) {
                regs->pc += 8;
                seq = INT_SEQ_SYSCALL;
            } else {
                seq = INT_SEQ_PANIC;
            }
            break;
        case EXCEPT_CODE_ECALL_U:
        case EXCEPT_CODE_ECALL_S:
            regs->pc += 4;
            seq = INT_SEQ_SYSCALL;
            break;
        case EXCEPT_CODE_INSTR_PAGE_FAULT:
        case EXCEPT_CODE_LOAD_PAGE_FAULT:
        case EXCEPT_CODE_STORE_PAGE_FAULT:
            seq = INT_SEQ_PAGE_FAULT;
            read_stval(error_code);
            break;
        default:
            seq = INT_SEQ_PANIC;
            break;
        }
    }

    panic_if(seq == INT_SEQ_PANIC,
             "Panic int seq! Int: %d code: %d, PC @ %lx, SP @ %lx, A0: %x\n",
             cause.interrupt, cause.except_code, regs->pc, regs->sp, regs->a0);

    // Go to the generic handler!
    struct int_context intc;
    intc.vector = seq;
    intc.error_code = error_code;
    intc.regs = regs;

    int_handler(seq, &intc);

    // Restore back to previous context
    set_local_int_state(1);
    restore_context_gpr(regs);

    unreachable();
}


/*
 * Initialization
 */
extern void raw_int_entry();

void init_int_entry_mp()
{
    // Remember MP
    struct mp_context *mp_ctxt = NULL;
    read_sscratch(mp_ctxt);
    ulong mp_id = mp_ctxt->id;
    ulong mp_seq = mp_ctxt->seq;

    // Set up int ctxt and make sscratch point to it
    struct reg_context *int_ctxt = get_cur_int_reg_context();
    int_ctxt->mp.id = mp_id;
    int_ctxt->mp.seq = mp_seq;
    write_sscratch(int_ctxt);

    kprintf("int ctxt @ %p, int_ctxt, mp id: %lx\n", int_ctxt, mp_id);

    // Set up handler entry
    ulong base = (ulong)(void *)&raw_int_entry;

    struct trap_vec_base_addr_reg tvec = { .value = 0 };
    tvec.mode = TVEC_MODE_DIRECT;
    tvec.base2 = base >> 2;

    write_stvec(tvec.value);
}

void init_int_entry()
{
    init_int_entry_mp();
}
