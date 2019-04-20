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
    mrs     x1, mpidr_el1
    and     x1, x1, #0x3
    cbnz    x1, __core_wait

    // Set up stack
    ldr     x1, =_start
    sub     sp, x1, 0x10

    // Go to loader
    bl      loader_entry

    // Should never reach here
__stop:
    b       __stop
    nop

__core_wait:
    b       .
    nop

_start_ap:
    // Set up stack
    ldr     x1, =_start
    sub     sp, x1, 0x10

    // Go to loader
    bl      loader_entry_ap

    // Should never reach here
    b       .
    nop
