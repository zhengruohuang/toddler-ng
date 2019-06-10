#define BOOT_STACK_SIZE 8192


.section ".text.boot"


.global _start
.global _start_ap

.extern loader_entry
.extern loader_entry_ap


_start:
    /* Set up machine state */
    li x1, 0
    li x2, 0
    li x3, 0
    li x4, 0
    li x5, 0
    li x6, 0
    li x7, 0
    li x8, 0
    li x9, 0
    li x10, 1       // a0 -> cfg_type
    //li x11, 0     // a1 -> cfg
    li x12, 0
    li x13, 0
    li x14, 0
    li x15, 0
    li x16, 0
    li x17, 0
    li x18, 0
    li x19, 0
    li x20, 0
    li x21, 0
    li x22, 0
    li x23, 0
    li x24, 0
    li x25, 0
    li x26, 0
    li x27, 0
    li x28, 0
    li x29, 0
    li x30, 0
    li x31, 0

    /* Set up stack, create stack frame */
    la sp, boot_stack + BOOT_STACK_SIZE
    addi sp, sp, -16

    /* Go to C */
    j loader_entry

    // Should never reach here
__stop:
    j       __stop
    nop


_start_ap:
    // Should never reach here
    j       .
    nop


boot_stack:
    .space BOOT_STACK_SIZE
