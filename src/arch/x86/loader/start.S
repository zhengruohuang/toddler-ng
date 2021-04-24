#include "common/include/asm.h"


.global _start
.global _start_mp

.global _stack_limit
.global _stack_limit_mp

.extern loader_entry
.extern loader_entry_mp

.extern loader_boot
.extern loader_boot_mp


.section ".data.stack"

.balign 16
_stack_limit_mp:
    .fill 1024, 1, 0
_stack_top_mp:

_stack_limit:
    .fill 7168, 1, 0
_stack_top:
    .fill 16, 1, 0


.section ".text.boot"

_start:
    /* Set up stack */
    mov_ul  $(_stack_top), %sp_ul
    sub_ul  $16, %sp_ul

#ifdef ARCH_IA32
    /* Set up arguments from multiboot */
    /* so that they can be accessed through both reg and stac*/
    pushl   %edx    /* Pointer to multiboot info */
    pushl   %eax    /* Magic */
#endif

    /* Go to loader */
    call    loader_entry

    /* Should never reach here */
    jmp     .
    nop

_start_mp:
    /* Set up stack */
    mov_ul  $(_stack_top_mp), %sp_ul
    sub_ul  $16, %sp_ul

    /* Go to loader */
    call    loader_entry_mp

    /* Should never reach here */
    jmp     .
    nop
