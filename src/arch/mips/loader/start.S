#include "common/include/asm.h"

.set    noreorder

.global _start
.global _start_mp
.global _start_mp_flag

.global _stack_limit
.global _stack_limit_mp

.global _cpunum_table

.extern loader_entry
.extern loader_entry_mp


.section ".data.stack"

.balign 16
_stack_limit_mp:
    .fill 1024, 1, 0
_stack_top_mp:

_stack_limit:
    .fill 7168, 1, 0
_stack_top:
    .fill 16, 1, 0


.section ".data.mp"

.balign 16
_cpunum_table:
    .fill 4096, 1, 0


.section ".text.boot"

.balign 8
_start:
    /* Get CPUnum (EBase, CP0 reg 15 sel 1) -> t1 */
    mfc0_ul t1, $15, 1
    andi    t1, t1, 0x3ff

    /* Register current CPU in cpunum bool table */
    la_ul   t2, _cpunum_table
    addu_ul t2, t1, t2
    li      t3, 1
    sb      t3, 0(t2)
    nop
    sync

    /* Redirect to MP startup rountine if CPUnum != 0 */
    bne     t1, zero, _start_mp
    nop

    /* Set up stack */
    la_ul   sp, _stack_top
    addi_ul sp, sp, -0x10

    /* Jump to loader entry */
    la_ul   t9, loader_entry
    jr      t9
    nop

    /* Should never reach here */
    b       .
    nop


.balign  8
_start_mp_flag:
    .fill 8, 1, 0xff

.balign 8
_start_mp:
    /* Get flag addr */
    la_ul   t4, _start_mp_flag

    /* Wait for start signal */
_check_flag:
    sync
    ld_ul   t5, 0(t4)
    bne     t1, t5, _check_flag
    nop

    /* Set up stack */
    la_ul   sp, _stack_top_mp
    addi_ul sp, sp, -0x10

    /* Jump to loader entry */
    la_ul   t9, loader_entry_mp
    jr      t9
    nop

    /* Should never reach here */
    b       .
    nop


// Deprecated
// .align  2
// .type _start, @function
// .ent _start
// _start:
//     .frame  sp, 0, ra
//
//     // Set up PIC code
//     // This will set up t9 to the address of cpload
//     jal     __set_up_pic
//     nop
//
//     // Set up GP
//     .cpload t9
//
//     // Set up stack
//     la_ul   sp, _stack_top
//     addi_ul sp, sp, -0x10
//
//     // Jump to loader entry
//     la_ul   t9, loader_entry
//     jr      t9
//     nop
//
//     // Should never reach here
// __stop:
//     b       .
//     nop
//
// __set_up_pic:
//     addi_ul t9, ra, 0
//     jr      ra
//     nop
//
// .end _start
