#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "common/include/atomic.h"
#include "common/include/msr.h"
#include "hal/include/kprintf.h"
#include "hal/include/setup.h"
#include "hal/include/lib.h"
#include "hal/include/mp.h"


void restore_context_gpr_from_int_stack()
{
    ulong *cur_stack_top = get_per_cpu(ulong, int_stack_top);
    struct reg_context *regs = (void *)*cur_stack_top;

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
      //"lw     x0,    0(x2);"
        "lw     x1,    4(x2);"
      //"lw     x2,    8(x2);"
        "lw     x3,   12(x2);"
        "lw     x4,   16(x2);"
        "lw     x5,   20(x2);"
        "lw     x6,   24(x2);"
        "lw     x7,   28(x2);"
        "lw     x8,   32(x2);"
        "lw     x9,   36(x2);"

        "lw     x10,  40(x2);"
        "lw     x11,  44(x2);"
        "lw     x12,  48(x2);"
        "lw     x13,  52(x2);"
        "lw     x14,  56(x2);"
        "lw     x15,  60(x2);"
        "lw     x16,  64(x2);"
        "lw     x17,  68(x2);"
        "lw     x18,  72(x2);"
        "lw     x19,  76(x2);"

        "lw     x20,  80(x2);"
        "lw     x21,  84(x2);"
        "lw     x22,  88(x2);"
        "lw     x23,  92(x2);"
        "lw     x24,  96(x2);"
        "lw     x25, 100(x2);"
        "lw     x26, 104(x2);"
        "lw     x27, 108(x2);"
        "lw     x28, 112(x2);"
        "lw     x29, 116(x2);"

        "lw     x30, 120(x2);"
        "lw     x31, 124(x2);"

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
    ulong *cur_stack_top = get_per_cpu(ulong, int_stack_top);
    struct reg_context *int_stack_top_regs = (void *)*cur_stack_top;
    panic_if(regs != int_stack_top_regs, "Bad reg context @ %p\n", regs);

    restore_context_gpr_from_int_stack();
}
