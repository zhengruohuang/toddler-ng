.section ".text.boot"

.global _start

.extern hal_entry

// r0 -> Loadr args
// r1 -> MP
_start:
    // Go to HAL
    bl      hal_entry

    // Should never reach here
__stop:
    b       __stop
    nop
