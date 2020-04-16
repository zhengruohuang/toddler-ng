#include "common/include/asm.h"


.set    noreorder


.global _start
.global _start_mp

.extern loader_entry
.extern loader_entry_mp


.section ".data.stack"

.align 16
_stack_limit:
    .fill 1024, 1, 0

_stack_top_mp:
    .fill 7168, 1, 0

_stack_top:
    .fill 16, 1, 0


.section ".text.boot"

.align  2
.type _start, @function
.ent _start
_start:
    .frame  sp, 0, ra

    // Set up PIC code
    // This will set up t9 to the address of cpload
    bal     __set_up_pic
    nop

    // Set up GP
    .cpload t9

    // Set up stack
    la      sp, _stack_top
    addi    sp, sp, -0x10
    //addi    sp, t9, -0x10

    // Jump to loader entry
    la      t9, loader_entry
    jalr    t9
    nop

    // Should never reach here
__stop:
    b       .
    nop

__set_up_pic:
    addi    t9, ra, 0
    jr      ra
    nop

.end _start
