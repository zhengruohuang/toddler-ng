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

// r15 -> Execution start addr
// r0 -> Zero
// r1 -> Machine ID
// r2 -> ATAGS/FDT start addr
_start:
    // Caches shouldn't be enabled at this point

    // Take care of multicore
    mrc p15, 0, r5, c0, c0, 5   // R5 = Multiprocessor Affinity Register (MPIDR)
    ands r5, #3                 // R5 = CPU ID (Bits 0..1)
    bne __mp_wait               // IF (CPU ID != 0) Branch To Infinite Loop (Core ID 1..3)

    // Set up stack
    //ldr     sp, =LOADER_BASE - 0x10
    //ldr     sp, =_start
    ldr     sp, =_stack_top
    sub     sp, sp, #0x10

    // Set up PC
    bl      __setup_pc

    // Go to loader
    bl      loader_entry

    // Should never reach here
    b       .
    nop

_start_mp:
    // Set up stack
    //ldr     sp, =_start
    ldr     sp, =_stack_top_mp
    sub     sp, sp, #0x10

    // Set up PC
    bl      __setup_pc

    // Go to loader
    bl      loader_entry_mp

    // Should never reach here
    b       .
    nop

__setup_pc:
    push    {lr}
    pop     {pc}

__mp_wait:
    b       .
    nop
