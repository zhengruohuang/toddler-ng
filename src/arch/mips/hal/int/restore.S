#include "common/include/asm.h"

#if (ARCH_WIDTH == 32)
#define WORD_SIZE   4

#elif (ARCH_WIDTH == 64)
#define WORD_SIZE   8

#else
#error "Unsupported arch width"
#endif

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
    /* lw      zero, (WORD_SIZE * 0)(k1) */
    ld_ul   AT, (WORD_SIZE * 1)(k1)
    ld_ul   v0, (WORD_SIZE * 2)(k1)
    ld_ul   v1, (WORD_SIZE * 3)(k1)
    ld_ul   a0, (WORD_SIZE * 4)(k1)
    ld_ul   a1, (WORD_SIZE * 5)(k1)
    ld_ul   a2, (WORD_SIZE * 6)(k1)
    ld_ul   a3, (WORD_SIZE * 7)(k1)
    ld_ul   t0, (WORD_SIZE * 8)(k1)
    ld_ul   t1, (WORD_SIZE * 9)(k1)
    ld_ul   t2, (WORD_SIZE * 10)(k1)
    ld_ul   t3, (WORD_SIZE * 11)(k1)
    ld_ul   t4, (WORD_SIZE * 12)(k1)
    ld_ul   t5, (WORD_SIZE * 13)(k1)
    ld_ul   t6, (WORD_SIZE * 14)(k1)
    ld_ul   t7, (WORD_SIZE * 15)(k1)
    ld_ul   t8, (WORD_SIZE * 16)(k1)
    ld_ul   t9, (WORD_SIZE * 17)(k1)
    ld_ul   s0, (WORD_SIZE * 18)(k1)
    ld_ul   s1, (WORD_SIZE * 19)(k1)
    ld_ul   s2, (WORD_SIZE * 20)(k1)
    ld_ul   s3, (WORD_SIZE * 21)(k1)
    ld_ul   s4, (WORD_SIZE * 22)(k1)
    ld_ul   s5, (WORD_SIZE * 23)(k1)
    ld_ul   s6, (WORD_SIZE * 24)(k1)
    ld_ul   s7, (WORD_SIZE * 25)(k1)
    ld_ul   k0, (WORD_SIZE * 26)(k1)
    /* lw      k1, (WORD_SIZE * 27)(k1) */
    ld_ul   gp, (WORD_SIZE * 28)(k1)
    ld_ul   sp, (WORD_SIZE * 29)(k1)
    ld_ul   fp, (WORD_SIZE * 30)(k1)
    ld_ul   ra, (WORD_SIZE * 31)(k1)

    /* Clear k1 for extra safety */
    xor     k1, k1, k1

    /* Return to user program */
    eret
    nop
