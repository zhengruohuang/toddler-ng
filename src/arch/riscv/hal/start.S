.extern hal_entry
.global _start

.section ".text.boot"

.balign  8
_start:
    jal hal_entry

    /* Should never reach here */
__stop:
    j       __stop
    nop
