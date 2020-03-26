.section ".text.boot"

.global _start

.extern hal

_start:
    // Go to kernel
    b       kernel_entry

    // Should never reach here
__stop:
    b       __stop
    nop
