#include "common/include/mach.h"
#include "common/include/abi.h"


.global _start
.global _stack_top

.extern _trap_table
.extern _overflow_registers
.extern _overflow_registers_offset


.section ".data.stack"

.align 16
_stack_limit:
    .fill 8192, 1, 0

.align 16
_stack_top:
    .fill 16, 1, 0


.section ".text.boot"

.macro find_symbol_abs_addr symbol, rd
#if (POSITION_INDEP)
    set _GLOBAL_OFFSET_TABLE_, %l7
    call __add_pc_to_l7
    add %l7, \symbol, %l7
    ld [%l7 + -4], \rd
#else
    set \symbol, \rd
#endif
.endm

_start:
    // Set stack
    find_symbol_abs_addr _stack_top, %sp

    // Set trap table
    find_symbol_abs_addr _trap_table, %l1
    mov %l1, %tbr

    // Set windows invalid mask
    // Set the current window as WIM as this window is no longer needed
    // after we jump into loader_entry
    mov %psr, %l1
    and %l1, 0x1f, %l1
    set 1, %l2
    sll %l2, %l1, %l2
    mov %l2, %wim

    // set 0x2, %g1            // WIM[1] = 1, trap on window #1
    // mov %g1, %wim

    // Set processor state
    mov %psr, %l1
    or %l1, 0x20, %l1       // Enable trap
    or %l1, 0xf00, %l1      // Interrupt level
    mov %l1, %psr

    // Set overflow register buffer offset
    find_symbol_abs_addr _overflow_registers, %l1
    find_symbol_abs_addr _overflow_registers_offset, %l2
    st %l1, [%l2]

    // Go to C
    call loader_entry
    nop

    // Stop
    b .
    nop

#if (POSITION_INDEP)
__add_pc_to_l7:
    retl
    add  %o7, %l7, %l7
#endif
