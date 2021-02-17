#include "common/include/asm.h"

.set noreorder
.set noat

.global restore_context_gpr

/*
 * restore_context_gpr(ulong ctxt_base)
 */
restore_context_gpr:
    /* Load ctxt base to k1 */
    move    k1, a0

    /* Restore all GPRs */
    /* lw      zero, 0(k1) */
    lw      AT, 4(k1)

    lw      v0, 8(k1)
    lw      v1, 12(k1)
    lw      a0, 16(k1)
    lw      a1, 20(k1)
    lw      a2, 24(k1)
    lw      a3, 28(k1)
    lw      t0, 32(k1)
    lw      t1, 36(k1)
    lw      t2, 40(k1)
    lw      t3, 44(k1)
    lw      t4, 48(k1)
    lw      t5, 52(k1)
    lw      t6, 56(k1)
    lw      t7, 60(k1)
    lw      t8, 64(k1)
    lw      t9, 68(k1)
    lw      s0, 72(k1)
    lw      s1, 76(k1)
    lw      s2, 80(k1)
    lw      s3, 84(k1)
    lw      s4, 88(k1)
    lw      s5, 92(k1)
    lw      s6, 96(k1)
    lw      s7, 100(k1)
    lw      k0, 104(k1)
    /* lw      k1, 108(k1) */
    lw      gp, 112(k1)
    lw      sp, 116(k1)
    lw      fp, 120(k1)
    lw      ra, 124(k1)

    /* Clear k1 for extra safety */
    xor     k1, k1, k1

    /* Return to user program */
    eret
    nop
