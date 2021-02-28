#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "common/include/page.h"
#include "common/include/msr.h"
#include "hal/include/setup.h"
#include "hal/include/kprintf.h"


void restore_context_gpr_from_shadow1()
{
    __asm__ __volatile__ (
        /* Move ctxt1 regs to ctxt0 */
        "l.mfspr r0, r0, 1056 + 0;"
        "l.mfspr r1, r0, 1056 + 1;"
        "l.mfspr r2, r0, 1056 + 2;"
        "l.mfspr r3, r0, 1056 + 3;"
        "l.mfspr r4, r0, 1056 + 4;"
        "l.mfspr r5, r0, 1056 + 5;"
        "l.mfspr r6, r0, 1056 + 6;"
        "l.mfspr r7, r0, 1056 + 7;"
        "l.mfspr r8, r0, 1056 + 8;"
        "l.mfspr r9, r0, 1056 + 9;"
        "l.mfspr r10, r0, 1056 + 10;"
        "l.mfspr r11, r0, 1056 + 11;"
        "l.mfspr r12, r0, 1056 + 12;"
        "l.mfspr r13, r0, 1056 + 13;"
        "l.mfspr r14, r0, 1056 + 14;"
        "l.mfspr r15, r0, 1056 + 15;"
        "l.mfspr r16, r0, 1056 + 16;"
        "l.mfspr r17, r0, 1056 + 17;"
        "l.mfspr r18, r0, 1056 + 18;"
        "l.mfspr r19, r0, 1056 + 19;"
        "l.mfspr r20, r0, 1056 + 20;"
        "l.mfspr r21, r0, 1056 + 21;"
        "l.mfspr r22, r0, 1056 + 22;"
        "l.mfspr r23, r0, 1056 + 23;"
        "l.mfspr r24, r0, 1056 + 24;"
        "l.mfspr r25, r0, 1056 + 25;"
        "l.mfspr r26, r0, 1056 + 26;"
        "l.mfspr r27, r0, 1056 + 27;"
        "l.mfspr r28, r0, 1056 + 28;"
        "l.mfspr r29, r0, 1056 + 29;"
        "l.mfspr r30, r0, 1056 + 30;"
        "l.mfspr r31, r0, 1056 + 31;"

        /* Return from exception */
        "l.rfe;"
        "l.nop;"
        :
        :
        : "memory"
    );
}

void restore_context_gpr(struct reg_context *regs)
{
    for (int i = 0; i < 32; i++) {
        ulong val = regs->regs[i];
        write_gpr1(val, i);
    }

    write_epcr0(regs->pc);
    write_esr0(regs->sr);

    // Delay slot
    if (regs->delay_slot) {
        struct supervision_reg sr;
        read_sr(sr.value);
        sr.delay_slot_except = 1;
        write_sr(sr.value);
    }

    restore_context_gpr_from_shadow1();
}

