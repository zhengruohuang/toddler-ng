.global _start
//.global _start_mp

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

_start:
    // Set up stack
    mov.l _boot_stack_top_addr, r15

    // Go to loader
    mov.l   _boot_entry_addr, r8
    jmp     @r8
    nop

    // Should never reach here
    bra     .
    nop

.align 4
_boot_stack_top_addr:
    .long _stack_top

_boot_entry_addr:
    .long loader_entry
