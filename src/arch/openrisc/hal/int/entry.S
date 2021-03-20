#include "hal/include/vecnum.h"


.extern tlb_miss_handler_entry
.extern int_handler_entry
.global raw_int_entry_base


/*
 * Helper macros
 */
.macro SAVE_REGS_TO_CTXT1
    /* Save LR (R9) as it will be overwritten during the func call */
    l.mtspr r0, r9, 1024 + 32 + 9

    /* Save other regs */
    l.jal   _save_regs_to_ctxt1
    l.nop
.endm

.macro CALL_LOADER_EXCEPT ctxt entry vec
    /* Load stack tops from ctxt2 */
    l.mfspr r14, r0, 1024 + 64 + 14
    l.mfspr r16, r0, 1024 + 64 + 16

    /* Set up stack */
    l.or    r1, r0, \ctxt
    l.addi  r1, r1, -0x10

    /* Set up param and go to the handler */
    l.ori   r3, r0, \vec
    l.or    r4, r0, \ctxt
    l.jal   \entry
    l.nop
.endm

.macro UNREACHEABLE
    l.j     .
    l.nop
.endm

.macro HANDLE_EXCEPT vec
    SAVE_REGS_TO_CTXT1
    CALL_LOADER_EXCEPT r16 int_handler_entry \vec
    UNREACHEABLE
.endm

.macro HANDLE_TLB_EXCEPT tlb_vec pf_vec
    SAVE_REGS_TO_CTXT1
    CALL_LOADER_EXCEPT r14 tlb_miss_handler_entry \tlb_vec
    CALL_LOADER_EXCEPT r16 int_handler_entry \pf_vec
    UNREACHEABLE
.endm


/*
 * Raw exception entry points
 */
raw_int_entry_base:
/*
 * Unknown
 */
.balign 8192
    HANDLE_EXCEPT 0x0

/*
 * 0x100: Reset
 */
.balign 256
    HANDLE_EXCEPT 0x1

/*
 * 0x200: Bus error
 */
.balign 256
    HANDLE_EXCEPT 0x2

/*
 * 0x300: Data page fault
 */
.balign 256
    HANDLE_EXCEPT 0x3

/*
 * 0x400: Instr page fault
 */
.balign 256
    HANDLE_EXCEPT 0x4

/*
 * 0x500: Tick timer
 */
.balign 256
    HANDLE_EXCEPT 0x5

/*
 * 0x600: Alignment
 */
.balign 256
    HANDLE_EXCEPT 0x6

/*
 * 0x700: Illegal instr
 */
.balign 256
    HANDLE_EXCEPT 0x7

/*
 * 0x800: External interrupt
 */
.balign 256
    HANDLE_EXCEPT 0x8

/*
 * 0x900: DTLB miss
 */
.balign 256
    HANDLE_TLB_EXCEPT 0x9 0x3

/*
 * 0xa00: ITLB miss
 */
.balign 256
    HANDLE_TLB_EXCEPT 0xa 0x4

/*
 * 0xb00: Range
 */
.balign 256
    HANDLE_EXCEPT 0xb

/*
 * 0xc00: Syscall
 */
.balign 256
    HANDLE_EXCEPT 0xc

/*
 * 0xd00: Floating point
 */
.balign 256
    HANDLE_EXCEPT 0xd

/*
 * 0xe00: Trap
 */
.balign 256
    HANDLE_EXCEPT 0xe

/*
 * 0xf00: Reserved
 */
.balign 256
    HANDLE_EXCEPT 0xf


/*
 * Save and restore registers
 */
.balign 256
_save_regs_to_ctxt1:
    /* Save ctxt0 regs to ctxt1 */
    l.mtspr r0, r0, 1024 + 32 + 0
    l.mtspr r0, r1, 1024 + 32 + 1
    l.mtspr r0, r2, 1024 + 32 + 2
    l.mtspr r0, r3, 1024 + 32 + 3
    l.mtspr r0, r4, 1024 + 32 + 4
    l.mtspr r0, r5, 1024 + 32 + 5
    l.mtspr r0, r6, 1024 + 32 + 6
    l.mtspr r0, r7, 1024 + 32 + 7
    l.mtspr r0, r8, 1024 + 32 + 8
 /* l.mtspr r0, r9, 1024 + 32 + 9 */ /* LR */
    l.mtspr r0, r10, 1024 + 32 + 10
    l.mtspr r0, r11, 1024 + 32 + 11
    l.mtspr r0, r12, 1024 + 32 + 12
    l.mtspr r0, r13, 1024 + 32 + 13
    l.mtspr r0, r14, 1024 + 32 + 14
    l.mtspr r0, r15, 1024 + 32 + 15
    l.mtspr r0, r16, 1024 + 32 + 16
    l.mtspr r0, r17, 1024 + 32 + 17
    l.mtspr r0, r18, 1024 + 32 + 18
    l.mtspr r0, r19, 1024 + 32 + 19
    l.mtspr r0, r20, 1024 + 32 + 20
    l.mtspr r0, r21, 1024 + 32 + 21
    l.mtspr r0, r22, 1024 + 32 + 22
    l.mtspr r0, r23, 1024 + 32 + 23
    l.mtspr r0, r24, 1024 + 32 + 24
    l.mtspr r0, r25, 1024 + 32 + 25
    l.mtspr r0, r26, 1024 + 32 + 26
    l.mtspr r0, r27, 1024 + 32 + 27
    l.mtspr r0, r28, 1024 + 32 + 28
    l.mtspr r0, r29, 1024 + 32 + 29
    l.mtspr r0, r30, 1024 + 32 + 30
    l.mtspr r0, r31, 1024 + 32 + 31

    l.jr    r9
    l.nop
