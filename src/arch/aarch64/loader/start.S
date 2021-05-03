.global _start
.global _start_mp

.global _stack_limit
.global _stack_limit_mp

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


.section ".text.boot"

/*
 * r?  -> FDT start addr
 */
_start:
    /*
     * Caches shouldn't be enabled at this point
     */

    /* Take care of multicore */
    mrs     x1, mpidr_el1
    and     x1, x1, #0x3
    cbnz    x1, __mp_wait

    /* Set up stack */
    ldr     x1, =_stack_top
    sub     sp, x1, 0x10

    /* Go to loader */
    bl      loader_entry

    /* Should never reach here */
    b       .
    nop

__mp_wait:
    b       .
    nop

_start_mp:
    /* Set up stack */
    ldr     x1, =_stack_top_mp
    sub     sp, x1, 0x10

    /* Go to loader */
    bl      loader_entry_mp

    /* Should never reach here */
    b       .
    nop
