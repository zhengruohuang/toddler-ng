#include "common/include/asm.h"
#include "hal/include/vecnum.h"


#if (ARCH_WIDTH == 32)
#define WORD_SIZE   4
#define WORD_SHIFT  2

#elif (ARCH_WIDTH == 64)
#define WORD_SIZE   8
#define WORD_SHIFT  3

#else
#error "Unsupported arch width"
#endif


.set noreorder
.set noat

.extern int_handler_entry
.extern per_cpu_context_base_table

.global raw_int_entry_base


/*
 * Save and restore registers
 */
__save_registers_except_ra:
    /* st_ul zero, (WORD_SIZE * 0)(k1) */
    st_ul   AT, (WORD_SIZE * 1)(k1)
    st_ul   v0, (WORD_SIZE * 2)(k1)
    st_ul   v1, (WORD_SIZE * 3)(k1)
    st_ul   a0, (WORD_SIZE * 4)(k1)
    st_ul   a1, (WORD_SIZE * 5)(k1)
    st_ul   a2, (WORD_SIZE * 6)(k1)
    st_ul   a3, (WORD_SIZE * 7)(k1)
    st_ul   t0, (WORD_SIZE * 8)(k1)
    st_ul   t1, (WORD_SIZE * 9)(k1)
    st_ul   t2, (WORD_SIZE * 10)(k1)
    st_ul   t3, (WORD_SIZE * 11)(k1)
    st_ul   t4, (WORD_SIZE * 12)(k1)
    st_ul   t5, (WORD_SIZE * 13)(k1)
    st_ul   t6, (WORD_SIZE * 14)(k1)
    st_ul   t7, (WORD_SIZE * 15)(k1)
    st_ul   t8, (WORD_SIZE * 16)(k1)
    st_ul   t9, (WORD_SIZE * 17)(k1)
    st_ul   s0, (WORD_SIZE * 18)(k1)
    st_ul   s1, (WORD_SIZE * 19)(k1)
    st_ul   s2, (WORD_SIZE * 20)(k1)
    st_ul   s3, (WORD_SIZE * 21)(k1)
    st_ul   s4, (WORD_SIZE * 22)(k1)
    st_ul   s5, (WORD_SIZE * 23)(k1)
    st_ul   s6, (WORD_SIZE * 24)(k1)
    st_ul   s7, (WORD_SIZE * 25)(k1)
    /* st_ul k0, (WORD_SIZE * 26)(k1) */
    /* st_ul k1, (WORD_SIZE * 27)(k1) */
    st_ul   gp, (WORD_SIZE * 28)(k1)
    st_ul   sp, (WORD_SIZE * 29)(k1)
    st_ul   fp, (WORD_SIZE * 30)(k1)

    jr ra
    nop

__restore_registers_except_ra:
    /* Restore all GPRs */
    /* ld_ul     zero, (WORD_SIZE * 0)(k1) */
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
    /* ld_ul     k1, (WORD_SIZE * 27)(k1) */
    ld_ul   gp, (WORD_SIZE * 28)(k1)
    ld_ul   sp, (WORD_SIZE * 29)(k1)
    ld_ul   fp, (WORD_SIZE * 30)(k1)

    jr ra
    nop


/*
 * Generic handler
 */
 __int_generic_entry:
    /* Save context, ctxt base @ k1 */
    st_ul   ra, (WORD_SIZE * 31)(k1)
    jal     __save_registers_except_ra
    nop

    /* Prepare C arguments, a0: except, a1: ctxt */
    move    a0, k0
    move    a1, k1

    /* Switch stack, and reserve some space for extra safe */
    sub_ul  sp, k1, 16

    /* Go to C */
    jal     int_handler_entry
    nop

    /* Done handling exception, restore context, ctxt base @ k1 */
    jal     __restore_registers_except_ra
    nop
    ld_ul   ra, (WORD_SIZE * 31)(k1)

    /* Clear k1 for safety. k0 now points to TCB */
    xor     k1, k1, k1

    /* Return to user program */
    eret
    nop

    /* Should never reach here */
    j   .
    nop


/*
 * Raw exception handler entry
 */
.macro EXCEPT_ENTRY except_num
    /* Clear k0 and k1 */
    xor     k0, k0, k0
    xor     k1, k1, k1

    /* Load base addr of the per-CPU ctxt base table to k1 */
    la_ul   k1, per_cpu_context_base_table
    ld_ul   k1, 0(k1)

    /* Find out current MP seq and store to k0 */
    mfc0    k0, $4, 0           /* Read PTE Context - CP0 reg 4 sel 0 */
    srl     k0, k0, 23          /* Get PTEBase, which is used for storing MP seq */

    /* Load the ctxt baes for current CPU */
    sll_ul  k0, k0, WORD_SHIFT  /* Calculate offset - cpu_id * 4 (8 on 64-bit) */
    addu_ul k1, k1, k0          /* Addr of context - base + offset */
    ld_ul   k1, 0(k1)           /* Load */

    /* Jump to generic handler entry */
    li_ul   k0, \except_num
    j       __int_generic_entry
    nop

    /* Should never reach here */
    j       .
    nop
.endm

.align 12
raw_int_entry_base:

/* TLB refill */
__tlb_refill_entry:
    EXCEPT_ENTRY EXCEPT_NUM_TLB_REFILL

#if (ARCH_WIDTH == 64)
.align 7
__tlb_refill_entry64:
    EXCEPT_ENTRY EXCEPT_NUM_TLB64_REFILL
#endif

/* Cache error */
.align 8
__cache_error_entry:
    EXCEPT_ENTRY EXCEPT_NUM_CACHE_ERROR

/* Other exceptions */
.align 7
__general_except_entry:
    EXCEPT_ENTRY EXCEPT_NUM_GENERAL
