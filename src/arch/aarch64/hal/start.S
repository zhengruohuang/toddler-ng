.extern hal_entry
.global _start

.section ".text.boot"

.balign  16
_start:
    bl  hal_entry

    /* Should never reach here */
__stop:
    b   .
    nop
