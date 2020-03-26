.section ".text.boot"

.global _start

.extern hal

// r0 -> Loadr args
_start:
    // Go to loader
    bl      hal_entry

    // Should never reach here
__stop:
    b       __stop
    nop
