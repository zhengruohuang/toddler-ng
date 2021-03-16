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

/*
 * x10/a0 -> boot hart
 * x11/a1 -> fdt
 */
_start:
    /* Set up stack, create stack frame */
    la sp, _stack_top
    addi sp, sp, -16

    /* Go to C */
    j loader_entry

    /* Should never reach here */
    j _stop

_start_mp:
    /* Set up stack, create stack frame */
    la sp, _stack_top_mp
    addi sp, sp, -16

    /* Go to C */
    j loader_entry_mp

    /* Should never reach here */
    j _stop

_stop:
    wfi
    j _stop
    nop
