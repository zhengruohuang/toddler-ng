#include "common/include/asm.h"
#include "hal/include/vecnum.h"


.set noreorder
.set noat

.extern int_handler_entry
.extern per_cpu_context_base_table

.global raw_int_entry_base


/*
 * Save and restore registers
 */
__save_registers_except_ra:
    /* sw zero, 0(k1) */
    sw AT, 4(k1)
    sw v0, 8(k1)
    sw v1, 12(k1)
    sw a0, 16(k1)
    sw a1, 20(k1)
    sw a2, 24(k1)
    sw a3, 28(k1)
    sw t0, 32(k1)
    sw t1, 36(k1)
    sw t2, 40(k1)
    sw t3, 44(k1)
    sw t4, 48(k1)
    sw t5, 52(k1)
    sw t6, 56(k1)
    sw t7, 60(k1)
    sw t8, 64(k1)
    sw t9, 68(k1)
    sw s0, 72(k1)
    sw s1, 76(k1)
    sw s2, 80(k1)
    sw s3, 84(k1)
    sw s4, 88(k1)
    sw s5, 92(k1)
    sw s6, 96(k1)
    sw s7, 100(k1)
    /* sw k0, 104(k1) */
    /* sw k1, 108(k1) */
    sw gp, 112(k1)
    sw sp, 116(k1)
    sw fp, 120(k1)

    jr ra
    nop

__restore_registers_except_ra:
    /* lw zero, 0(k1) */
    lw AT, 4(k1)
    lw v0, 8(k1)
    lw v1, 12(k1)
    lw a0, 16(k1)
    lw a1, 20(k1)
    lw a2, 24(k1)
    lw a3, 28(k1)
    lw t0, 32(k1)
    lw t1, 36(k1)
    lw t2, 40(k1)
    lw t3, 44(k1)
    lw t4, 48(k1)
    lw t5, 52(k1)
    lw t6, 56(k1)
    lw t7, 60(k1)
    lw t8, 64(k1)
    lw t9, 68(k1)
    lw s0, 72(k1)
    lw s1, 76(k1)
    lw s2, 80(k1)
    lw s3, 84(k1)
    lw s4, 88(k1)
    lw s5, 92(k1)
    lw s6, 96(k1)
    lw s7, 100(k1)
    lw k0, 104(k1)
    /* lw k1, 108(k1) */
    lw gp, 112(k1)
    lw sp, 116(k1)
    lw fp, 120(k1)

    jr ra
    nop


/*
 * Generic handler
 */
 __int_generic_entry:
    /* Save context, ctxt base @ k1 */
    sw      ra, 124(k1)
    jal     __save_registers_except_ra
    nop

    /* Prepare C arguments, a0: except, a1: ctxt */
    move    a0, k0
    move    a1, k1

    /* Switch stack, and reserve some space for extra safe */
    sub     sp, k1, 16

    /* Go to C */
    jal     int_handler_entry
    nop

    /* Done handling exception, restore context, ctxt base @ k1 */
    jal     __restore_registers_except_ra
    nop
    lw      ra, 124(k1)

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
    /* Load base addr of the per-CPU ctxt base table to k1 */
    la      k1, per_cpu_context_base_table
    lw      k1, 0(k1)

    /* Find out current CPUnum and store to k0 */
    mfc0    k0, $15, 1      /* Read EBbase - CP0 reg 15 sel 1 */
    andi    k0, k0, 0x3ff   /* Get CPU ID */

    /* Load the ctxt baes for current CPU */
    sll     k0, k0, 2       /* Calculate offset - cpu_id * 4 */
    addu    k1, k1, k0      /* Addr of context - base + offset */
    lw      k1, 0(k1)       /* Load */

    /* Jump to generic handler entry */
    li      k0, \except_num
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

/* Cache error */
.align 8
__cache_error_entry:
    EXCEPT_ENTRY EXCEPT_NUM_CACHE_ERROR

/* Other exceptions */
.align 7
__general_except_entry:
    EXCEPT_ENTRY EXCEPT_NUM_GENERAL
