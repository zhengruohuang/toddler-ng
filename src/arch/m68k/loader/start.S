#include "common/include/mach.h"


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
    lea     %pc@(_stack_top), %sp

    // Go to loader
    jsr     loader_entry

    // Should never reach here
    jmp     .
    nop
