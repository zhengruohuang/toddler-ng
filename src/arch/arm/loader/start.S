#include "common/include/mach.h"

.section ".text.boot"

.global _start
.global _start_ap

.extern loader_entry
.extern loader_entry_ap

// r15 -> Execution start addr
// r0 -> Zero
// r1 -> Machine ID
// r2 -> ATAGS/FDT start addr
_start:
    // Caches shouldn't be enabled at this point

    // Take care of multicore
    mrc p15, 0, r5, c0, c0, 5     // R5 = Multiprocessor Affinity Register (MPIDR)
    ands r5, #3                   // R5 = CPU ID (Bits 0..1)
    bne __core_wait               // IF (CPU ID != 0) Branch To Infinite Loop (Core ID 1..3)

    // Set up stack
    //ldr     sp, =LOADER_BASE - 0x10
    ldr     sp, =_start
    sub     sp, sp, #0x10

    // Set up PC
    bl      __setup_pc

    // Go to loader
    bl      loader_entry

    // Should never reach here
__stop:
    b       __stop
    nop

__setup_pc:
    push    {lr}
    pop     {pc}

__core_wait:
    b       .
    nop

_start_ap:
    // Set up stack
    ldr     sp, =LOADER_BASE - 0x10

    // Go to loader
    bl      loader_entry_ap

    // Should never reach here
    b       .
    nop
