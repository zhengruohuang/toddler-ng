#include "common/include/abi.h"
#include "common/include/asm.h"


.altmacro

.global raw_int_begin
.global raw_int_end

.extern int_handler_prepare


/*
 * Helper macros
 */
generic_raw_int_handler:
    /* Save GPRs */
    push_ul %ax_ul
    push_ul %cx_ul
    push_ul %dx_ul
    push_ul %bx_ul
    push_ul %bp_ul
    push_ul %si_ul
    push_ul %di_ul

#if (ARCH_WIDTH == 64)
    push_ul %r15
    push_ul %r14
    push_ul %r13
    push_ul %r12
    push_ul %r11
    push_ul %r10
    push_ul %r9
    push_ul %r8
#endif

    /* Save segment registers */
    xor     %ax_ul, %ax_ul
    movw    %ds, %ax
    push_ul %ax_ul
    movw    %es, %ax
    push_ul %ax_ul
    movw    %fs, %ax
    push_ul %ax_ul
    movw    %gs, %ax
    push_ul %ax_ul
    movw    %ss, %ax
    push_ul %ax_ul

    /* Load kernel DS */
    mov_ul  $0x10, %ax_ul
    movw    %ax, %ds

    /* Go to C handler */
#if (ARCH_WIDTH == 64)
    mov_ul  %sp_ul, %rdi
#elif (ARCH_WIDTH == 32)
    mov_ul  %sp_ul, %ax_ul
    push_ul %ax_ul
#endif
    call    int_handler_prepare

    /* Unreachable */
    jmp     .
    nop

.macro INT_HANDLER_NO_CODE vec
    cli
    push_ul $0
    push_ul $\vec
    jmp     generic_raw_int_handler
.endm

.macro INT_HANDLER vec
    cli
    push_ul $\vec
    jmp     generic_raw_int_handler
.endm

.macro DEFAULT_INT_HANDLER vec
.balign 32
raw_int_handler_\vec:
    INT_HANDLER_NO_CODE \vec
.endm


/*
 * Predefined exception & trap handlers
 */
.balign 32
raw_int_begin:
raw_int_handler_divide:
    INT_HANDLER_NO_CODE 0

.balign 32
raw_int_handler_debug:
    INT_HANDLER_NO_CODE 1

.balign 32
raw_int_handler_nmi:
    INT_HANDLER_NO_CODE 2

.balign 32
raw_int_handler_breakpoint:
    INT_HANDLER_NO_CODE 3

.balign 32
raw_int_handler_overflow:
    INT_HANDLER_NO_CODE 4

.balign 32
raw_int_handler_bounds_check:
    INT_HANDLER_NO_CODE 5

.balign 32
raw_int_handler_invalid_opcode:
    INT_HANDLER_NO_CODE 6

.balign 32
raw_int_handler_coproc_unavail:
    INT_HANDLER_NO_CODE 7

.balign 32
raw_int_handler_double_fault:
    INT_HANDLER 8

.balign 32
raw_int_handler_coproc_seg_overrun:
    INT_HANDLER_NO_CODE 9

.balign 32
raw_int_handler_invalid_tss:
    INT_HANDLER 10

.balign 32
raw_int_handler_seg_not_present:
    INT_HANDLER 11

.balign 32
raw_int_handler_stack_exception:
    INT_HANDLER 12

.balign 32
raw_int_handler_general_protection:
    INT_HANDLER 13

.balign 32
raw_int_handler_page_fault:
    INT_HANDLER 14

/* Exception #15 is reserved */
.balign 32
raw_int_handler_15:
    INT_HANDLER_NO_CODE 15

.balign 32
raw_int_handler_x87_error:
    INT_HANDLER_NO_CODE 16

.balign 32
raw_int_handler_align_check:
    INT_HANDLER 17

.balign 32
raw_int_handler_machine_check:
    INT_HANDLER_NO_CODE 18

.balign 32
raw_int_handler_simd_error:
    INT_HANDLER_NO_CODE 19

.balign 32
raw_int_handler_virtualization_error:
    INT_HANDLER_NO_CODE 20

/* Exception #21 to #29 are reserved */
.set i, 21
.rept 9
    DEFAULT_INT_HANDLER %i
    .set i, i + 1
.endr

.balign 32
raw_int_handler_security_error:
    INT_HANDLER 30

/* Exception #31 is reserved */
.balign 32
raw_int_handler_31:
    INT_HANDLER_NO_CODE 31

/*
 * Interrupt handlers (vector = 32 - 255)
 */
.set i, 32
.rept 224
    DEFAULT_INT_HANDLER %i
    .set i, i + 1
.endr

.balign 32
raw_int_end:
