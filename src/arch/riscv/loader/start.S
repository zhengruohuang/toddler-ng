.global _start
.global _start_ap

.extern loader_entry
.extern loader_entry_ap


.section ".data.stack"

.align 16
_stack_limit:
    .fill 1024, 1, 0

_stack_top_mp:
    .fill 7168, 1, 0

_stack_top:
    .fill 16, 1, 0


.section ".text.boot"

/*
 * x10/a0 -> boot hart
 * x11/a1 -> fdt
 */
_start:
    /* Spin if not boot hart */
    bne x10, x0, _stop

    /* Set up stack, create stack frame */
    la sp, _stack_top
    addi sp, sp, -16

    /* Go to C */
    j loader_entry

    /* Should never reach here */
    wfi
    j .
    nop

_start_ap:
    /* Should never reach here */
    wfi
    j .
    nop

_stop:
    wfi
    j .
    nop
