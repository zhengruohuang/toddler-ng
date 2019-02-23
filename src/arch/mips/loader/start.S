#include "common/include/asm.h"

.section ".text.boot"

.set    noreorder

.extern loader_entry

.globl _start
.align  2
.type _start,@function
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
    addi    sp, t9, -0x10

    // Jump to loader entry
    la      t9, loader_entry
    jalr    t9
    nop

    // Should never reach here
__stop:
    b       __stop
    nop

__set_up_pic:
    addi    t9, ra, 0
    jr      ra
    nop

.end _start
