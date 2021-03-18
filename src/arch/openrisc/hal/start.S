.section ".text.boot"

.global _start

.extern hal_entry

// r0 -> Loadr args
// r1 -> MP
_start:
    // Go to HAL
    l.j     hal_entry
    l.nop

    // Should never reach here
__stop:
    l.j     __stop
    l.nop
