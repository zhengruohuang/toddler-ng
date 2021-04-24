.section ".text.boot"

.global _start

.extern kernel

_start:
    // Go to kernel
    jmp     kernel

    // Should never reach here
    jmp     .
    nop
