#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "common/include/atomic.h"
#include "common/include/msr.h"
#include "hal/include/kprintf.h"
#include "hal/include/setup.h"
#include "hal/include/lib.h"
#include "hal/include/int.h"
#include "hal/include/mp.h"


#define str(s) #s
#define eval(s) str(s)

#define WORD_SIZE       4
#define REG_BASE_OFFSET 8


void restore_context_gpr_from_int_stack()
{
    struct reg_context *regs = get_cur_int_reg_context();
    panic_if(regs->status & 0x3, "Bad sstatus!\n");

    // Save SP to sscratch
    write_sscratch(regs->sp);

    // Restore PC
    write_sepc(regs->pc);

    // Restore status
    write_sstatus(regs->status);

    __asm__ __volatile__ (
        /* Set up SP */
        "mv     x2, %[regs];"

        /* Restore registers */
      //"lw     x0,  " eval(REG_BASE_OFFSET + WORD_SIZE * 0) "(x2);"
        "lw     x1,  " eval(REG_BASE_OFFSET + WORD_SIZE * 1) "(x2);"
      //"lw     x2,  " eval(REG_BASE_OFFSET + WORD_SIZE * 2) "(x2);"
        "lw     x3,  " eval(REG_BASE_OFFSET + WORD_SIZE * 3) "(x2);"
        "lw     x4,  " eval(REG_BASE_OFFSET + WORD_SIZE * 4) "(x2);"
        "lw     x5,  " eval(REG_BASE_OFFSET + WORD_SIZE * 5) "(x2);"
        "lw     x6,  " eval(REG_BASE_OFFSET + WORD_SIZE * 6) "(x2);"
        "lw     x7,  " eval(REG_BASE_OFFSET + WORD_SIZE * 7) "(x2);"
        "lw     x8,  " eval(REG_BASE_OFFSET + WORD_SIZE * 8) "(x2);"
        "lw     x9,  " eval(REG_BASE_OFFSET + WORD_SIZE * 9) "(x2);"

        "lw     x10, " eval(REG_BASE_OFFSET + WORD_SIZE * 10) "(x2);"
        "lw     x11, " eval(REG_BASE_OFFSET + WORD_SIZE * 11) "(x2);"
        "lw     x12, " eval(REG_BASE_OFFSET + WORD_SIZE * 12) "(x2);"
        "lw     x13, " eval(REG_BASE_OFFSET + WORD_SIZE * 13) "(x2);"
        "lw     x14, " eval(REG_BASE_OFFSET + WORD_SIZE * 14) "(x2);"
        "lw     x15, " eval(REG_BASE_OFFSET + WORD_SIZE * 15) "(x2);"
        "lw     x16, " eval(REG_BASE_OFFSET + WORD_SIZE * 16) "(x2);"
        "lw     x17, " eval(REG_BASE_OFFSET + WORD_SIZE * 17) "(x2);"
        "lw     x18, " eval(REG_BASE_OFFSET + WORD_SIZE * 18) "(x2);"
        "lw     x19, " eval(REG_BASE_OFFSET + WORD_SIZE * 19) "(x2);"

        "lw     x20, " eval(REG_BASE_OFFSET + WORD_SIZE * 20) "(x2);"
        "lw     x21, " eval(REG_BASE_OFFSET + WORD_SIZE * 21) "(x2);"
        "lw     x22, " eval(REG_BASE_OFFSET + WORD_SIZE * 22) "(x2);"
        "lw     x23, " eval(REG_BASE_OFFSET + WORD_SIZE * 23) "(x2);"
        "lw     x24, " eval(REG_BASE_OFFSET + WORD_SIZE * 24) "(x2);"
        "lw     x25, " eval(REG_BASE_OFFSET + WORD_SIZE * 25) "(x2);"
        "lw     x26, " eval(REG_BASE_OFFSET + WORD_SIZE * 26) "(x2);"
        "lw     x27, " eval(REG_BASE_OFFSET + WORD_SIZE * 27) "(x2);"
        "lw     x28, " eval(REG_BASE_OFFSET + WORD_SIZE * 28) "(x2);"
        "lw     x29, " eval(REG_BASE_OFFSET + WORD_SIZE * 29) "(x2);"

        "lw     x30, " eval(REG_BASE_OFFSET + WORD_SIZE * 30) "(x2);"
        "lw     x31, " eval(REG_BASE_OFFSET + WORD_SIZE * 31) "(x2);"

        /* Restore SP and sscratch by swapping them */
        "csrrw  x2, sscratch, x2;"

        /* Return to target context */
        "sret;"
        "nop;"

        /* Should never reach here */
        "j      .;"
        "nop;"
        :
        : [regs] "r" ((ulong)regs)
        : "memory"
    );
}

void restore_context_gpr(struct reg_context *regs)
{
//     kprintf("Restore, CPU seq: %ld, ID: %lx\n",
//             arch_get_cur_mp_seq(), arch_get_cur_mp_id());

    struct reg_context *int_regs = get_cur_int_reg_context();
    panic_if(regs != int_regs, "Bad reg context @ %p, expected @ %p\n",
             regs, int_regs);

    restore_context_gpr_from_int_stack();
}
