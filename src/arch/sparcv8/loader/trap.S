#include "common/include/abi.h"


.global _trap_table
.global _overflow_registers
.global _overflow_registers_offset


#define TRAP_ENTRY_SIZE         16
#define TRAP_TABLE_ENTRY_COUNT  128    //256
#define TRAP_TABLE_SIZE         (TRAP_ENTRY_SIZE * TRAP_TABLE_ENTRY_COUNT)


.section ".data.overflow"

.align 16
_overflow_registers:
    .fill 4096, 1, 0

.align 16
_overflow_registers_offset:
    .fill 16, 1, 0


.section ".text"

.macro find_symbol_abs_addr symbol, rd, save
#if (POSITION_INDEP)
    mov %o7, \save
    set _GLOBAL_OFFSET_TABLE_, %l7
    call __add_pc_to_l7
    add %l7, \symbol, %l7
    ld [%l7 + -4], \rd
    mov \save, %o7
#else
    set \symbol, \rd
#endif
.endm

#if (POSITION_INDEP)
__add_pc_to_l7:
    retl
    add  %o7, %l7, %l7
#endif

window_overflow_trap:
    // %l1 and %l2 contain return addresses and should not be touched

    // Save a couple of global registers to local
    mov %g3, %l3
    mov %g4, %l4
    mov %g5, %l5
    mov %g6, %l6

    // %l0 and %l7 are free and will be used as a temp register

    // Mute WIM
    mov %g0, %wim
    nop
    nop
    nop

    // Find out where to save the registers and store it to %g3
    // Before and after the macro, we need to save and store %o7
    find_symbol_abs_addr _overflow_registers_offset, %g4, %g5
    ld [%g4], %g3

    // Go to next window, which is to be saved
    save

    // Save all input and local registers
    std %l0, [%g3 + 0]
    std %l2, [%g3 + 8]
    std %l4, [%g3 + 16]
    std %l6, [%g3 + 24]
    std %i0, [%g3 + 32]
    std %i2, [%g3 + 40]
    std %i4, [%g3 + 48]
    std %i6, [%g3 + 56]
    add %g3, 64, %g3

    // Save updated overflow offset (%g3)
    st %g3, [%g4]

    // Set the bit in WIM (currently stored in %g6) for current window
    // This is to capture the next overflow
    mov %psr, %g4
    and %g4, 0x1f, %g4
    set 1, %g5
    sll %g5, %g4, %g6

    // Back to previous window, or the window we came from
    restore

    // Set the new WIM
    mov %g6, %wim
    nop
    nop
    nop

    // Restore global registers
    mov %l3, %g3
    mov %l4, %g4
    mov %l5, %g5
    mov %l6, %g6

    // Return from trap
    jmp %l1
    rett %l2

    b .
    nop

window_underflow_trap:
    // %l1 and %l2 contain return addresses and should not be touched

    // Save a couple of global registers to local
    mov %g3, %l3
    mov %g4, %l4
    mov %g5, %l5
    mov %g6, %l6

    // %l0 and %l7 are free and will be used as a temp register

    // Mute WIM
    mov %g0, %wim
    nop
    nop
    nop

    // Find out where to restore the registers from and store it to %g3
    // Before and after the macro, we need to save and store %o7
    find_symbol_abs_addr _overflow_registers_offset, %g4, %g5
    ld [%g4], %g3

    // Go to the previous 2 window, which is to be restored
    restore
    restore

    // Restore all input and local registers
    sub %g3, 64, %g3
    ldd [%g3 + 0], %l0
    ldd [%g3 + 8], %l2
    ldd [%g3 + 16], %l4
    ldd [%g3 + 24], %l6
    ldd [%g3 + 32], %i0
    ldd [%g3 + 40], %i2
    ldd [%g3 + 48], %i4
    ldd [%g3 + 56], %i6

    // Save updated overflow offset (%g3)
    st %g3, [%g4]

    // Set the bit in WIM (currently stored in %g6) for current window
    // This is to capture the next underflow
    restore
    mov %psr, %g4
    and %g4, 0x1f, %g4
    set 1, %g5
    sll %g5, %g4, %g6
    save

    // Back to next 2 window, or the window we came from
    save
    save

    // Set the new WIM
    mov %g6, %wim
    nop
    nop
    nop

    // Restore global registers
    mov %l3, %g3
    mov %l4, %g4
    mov %l5, %g5
    mov %l6, %g6

    // Return from trap
    jmp %l1
    rett %l2

    b .
    nop

#define HANDLE(_vector, _handler)           \
    .align TRAP_ENTRY_SIZE;                 \
_trap_table_vector##_vector:;               \
    b _handler;                             \
    nop;

#define IGNORE(_vector)                     \
    .align TRAP_ENTRY_SIZE;                 \
    b .;                                    \
    nop;

.align 4096
_trap_table:
    IGNORE(0x0)
    IGNORE(0x1)
    IGNORE(0x2)
    IGNORE(0x3)
    IGNORE(0x4)
    HANDLE(0x5, window_overflow_trap)
    HANDLE(0x6, window_underflow_trap)
    IGNORE(0x7)
    IGNORE(0x8)
    IGNORE(0x9)
    IGNORE(0xa)
    IGNORE(0xb)
    IGNORE(0xc)
    IGNORE(0xd)
    IGNORE(0xe)
    IGNORE(0xf)
    IGNORE(0x10)
    IGNORE(0x11)
    IGNORE(0x12)
    IGNORE(0x13)
    IGNORE(0x14)
    IGNORE(0x15)
    IGNORE(0x16)
    IGNORE(0x17)
    IGNORE(0x18)
    IGNORE(0x19)
    IGNORE(0x1a)
    IGNORE(0x1b)
    IGNORE(0x1c)
    IGNORE(0x1d)
    IGNORE(0x1e)
    IGNORE(0x1f)
    IGNORE(0x21)
    IGNORE(0x22)
    IGNORE(0x23)
    IGNORE(0x24)
    IGNORE(0x25)
    IGNORE(0x26)
    IGNORE(0x27)
    IGNORE(0x28)
    IGNORE(0x29)
    IGNORE(0x2a)
    IGNORE(0x2b)
    IGNORE(0x2c)
    IGNORE(0x2d)
    IGNORE(0x2e)
    IGNORE(0x2f)
    IGNORE(0x30)
    IGNORE(0x31)
    IGNORE(0x32)
    IGNORE(0x33)
    IGNORE(0x34)
    IGNORE(0x35)
    IGNORE(0x36)
    IGNORE(0x37)
    IGNORE(0x38)
    IGNORE(0x39)
    IGNORE(0x3a)
    IGNORE(0x3b)
    IGNORE(0x3c)
    IGNORE(0x3d)
    IGNORE(0x3e)
    IGNORE(0x3f)
    IGNORE(0x40)
    IGNORE(0x41)
    IGNORE(0x42)
    IGNORE(0x43)
    IGNORE(0x44)
    IGNORE(0x45)
    IGNORE(0x46)
    IGNORE(0x47)
    IGNORE(0x48)
    IGNORE(0x49)
    IGNORE(0x4a)
    IGNORE(0x4b)
    IGNORE(0x4c)
    IGNORE(0x4d)
    IGNORE(0x4e)
    IGNORE(0x4f)
    IGNORE(0x50)
    IGNORE(0x51)
    IGNORE(0x52)
    IGNORE(0x53)
    IGNORE(0x54)
    IGNORE(0x55)
    IGNORE(0x56)
    IGNORE(0x57)
    IGNORE(0x58)
    IGNORE(0x59)
    IGNORE(0x5a)
    IGNORE(0x5b)
    IGNORE(0x5c)
    IGNORE(0x5d)
    IGNORE(0x5e)
    IGNORE(0x5f)
    IGNORE(0x60)
    IGNORE(0x61)
    IGNORE(0x62)
    IGNORE(0x63)
    IGNORE(0x64)
    IGNORE(0x65)
    IGNORE(0x66)
    IGNORE(0x67)
    IGNORE(0x68)
    IGNORE(0x69)
    IGNORE(0x6a)
    IGNORE(0x6b)
    IGNORE(0x6c)
    IGNORE(0x6d)
    IGNORE(0x6e)
    IGNORE(0x6f)
    IGNORE(0x70)
    IGNORE(0x71)
    IGNORE(0x72)
    IGNORE(0x73)
    IGNORE(0x74)
    IGNORE(0x75)
    IGNORE(0x76)
    IGNORE(0x77)
    IGNORE(0x78)
    IGNORE(0x79)
    IGNORE(0x7a)
    IGNORE(0x7b)
    IGNORE(0x7c)
    IGNORE(0x7d)
    IGNORE(0x7e)
    IGNORE(0x7f)
