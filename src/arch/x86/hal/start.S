.section ".text.boot"

.global _start

.extern hal_entry

_start:
    // Go to HAL
    jmp     hal_entry

    // Should never reach here
    jmp     .
    nop
