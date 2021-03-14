#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "common/include/atomic.h"
#include "common/include/msr.h"
#include "hal/include/kprintf.h"
#include "hal/include/setup.h"
#include "hal/include/lib.h"
#include "hal/include/int.h"
#include "hal/include/mp.h"


void restore_context_gpr_from_int_stack()
{
    struct reg_context *regs = get_cur_int_reg_context();

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
      //"lw     x0,    4(x2);"
        "lw     x1,    8(x2);"
      //"lw     x2,   12(x2);"
        "lw     x3,   16(x2);"
        "lw     x4,   20(x2);"
        "lw     x5,   24(x2);"
        "lw     x6,   28(x2);"
        "lw     x7,   32(x2);"
        "lw     x8,   36(x2);"
        "lw     x9,   40(x2);"

        "lw     x10,  44(x2);"
        "lw     x11,  48(x2);"
        "lw     x12,  52(x2);"
        "lw     x13,  56(x2);"
        "lw     x14,  60(x2);"
        "lw     x15,  64(x2);"
        "lw     x16,  68(x2);"
        "lw     x17,  72(x2);"
        "lw     x18,  76(x2);"
        "lw     x19,  80(x2);"

        "lw     x20,  84(x2);"
        "lw     x21,  88(x2);"
        "lw     x22,  92(x2);"
        "lw     x23,  96(x2);"
        "lw     x24, 100(x2);"
        "lw     x25, 104(x2);"
        "lw     x26, 108(x2);"
        "lw     x27, 112(x2);"
        "lw     x28, 116(x2);"
        "lw     x29, 120(x2);"

        "lw     x30, 124(x2);"
        "lw     x31, 128(x2);"

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
    struct reg_context *int_regs = get_cur_int_reg_context();
    panic_if(regs != int_regs, "Bad reg context @ %p\n", regs);

    restore_context_gpr_from_int_stack();
}
