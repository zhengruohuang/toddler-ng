.global _start
.global _start_mp
.global _start_mp_flag

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
    /* Get core ID */
    l.mfspr r3, r0, 128;
    l.sfeq  r3, r0
    l.bnf   _start_mp
    l.nop

    /* Set up stack */
    l.movhi r1, hi(_stack_top)
    l.ori   r1, r1, lo(_stack_top)
    l.addi  r1, r1, -0x10

    /* Jump to loader entry */
    l.jal   loader_entry
    l.nop

    /* Should never reach here */
    l.j     .
    l.nop


.balign  8
_start_mp_flag:
    .fill 8, 1, 0xff

_start_mp:
    /* Get flag addr */
    l.movhi r15, hi(_start_mp_flag)
    l.ori   r15, r1, lo(_start_mp_flag)

    /* Wait for start signal */
_check_flag:
    l.msync
    l.lwz   r17, 0(r15)
    l.sfeq  r3, r17
    l.bnf   _check_flag
    l.nop

    /* Set up stack */
    l.movhi r1, hi(_stack_top_mp)
    l.ori   r1, r1, lo(_stack_top_mp)
    l.addi  r1, r1, -0x10

    /* Jump to loader entry */
    l.jal   loader_entry_mp
    l.nop

    /* Should never reach here */
    l.j     .
    l.nop
