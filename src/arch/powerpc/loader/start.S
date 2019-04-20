#include "common/include/asm.h"

.global _start
.global _stack_top
.global call_real_mode
.global call_hal

.section ".text.boot"

_start:
    // Set up stack
    lis r1, _stack_top@ha
    addi r1, r1, _stack_top@l

    // Go to C!
    b loader_entry
    nop

    // Stop
    b .
    nop

.align 4
_stack_limit:
    .fill 8192, 1, 0

.align 4
_stack_top:
    .fill 16, 1, 0


// r3 = paddr of boot params
// r4 = paddr of "call_hal"
// r5 = paddr of real mode entry point
// r6 = paddr of stack top
.align 4
call_real_mode:
    // Disable interrupts
    mfmsr r31
    rlwinm r31, r31, 0, 17, 15
    mtmsr r31

    // Set real mode entry point physical address
    mtspr srr0, r5

    // Set stack top
    subi sp, r6, 0x10

    // Disable address translation and interrupts
    //mfmsr r31
    //lis r30, ~0@h
    //ori r30, r30, ~(msr_ir | msr_dr | msr_ee)@l
    //and r31, r31, r30
    li r31, 0
    mtspr srr1, r31

    // Go
    sync
    isync
    rfi

    // Stop
    b .
    nop

// r3 = paddr of boot params
// r4 = vaddr of HAL entry
.align 4
call_hal:
    // Set HAL entry point
    mtspr srr0, r4

    // Set HAL stack
    //subi sp, r4, 0x10

    // Set exception stack top
    //li r31, 0x100
    //mtsprg0 r31

    // This forces the usage of sprg0 as exception stack pointer
    //li sp, 0

    // Enable address translation
    mfmsr r31
    ori r31, r31, (msr_ir | msr_dr)@l
    mtspr srr1, r31

    // Go!
    sync
    isync
    rfi

    // Stop
    b .
    nop
