/* #include "common/include/asm.h" */


.macro SAVE_REGS_TO_CTXT1
    // Save LR (R9) and SP (R1) as they will be overwritten during the func call
    l.mtspr r0, r1, 1057
    l.mtspr r0, r9, 1065

    // Save other regs
    l.jal   _save_regs_to_ctxt1
    l.nop
.endm

.macro RESTORE_REGS_FROM_CTXT1
    // Restore other regs except LR (R9)
    l.jal   _restore_regs_from_ctxt1
    l.nop

    // Restore LR (R9) and SP (R1)
    l.mfspr r9, r0, 1065
    l.mfspr r1, r0, 1057
.endm

.macro CALL_LOADER_EXCEPT vec
    // Set up stack
    l.movhi r1, hi(_stack_top_mp)
    l.ori   r1, r1, lo(_stack_top_mp)
    l.addi  r1, r1, -0x10

    // Set up param and go to the handler
    l.ori   r3, r0, \vec
    l.jal   loader_except
    l.nop
.endm

.macro RETURN_FROM_EXCEPT
    l.rfe
    l.nop
.endm

.macro UNREACHEABLE
    l.j     .
    l.nop
.endm

.macro HANDLE_EXCEPT vec
    SAVE_REGS_TO_CTXT1
    CALL_LOADER_EXCEPT \vec
    RESTORE_REGS_FROM_CTXT1
    RETURN_FROM_EXCEPT
    UNREACHEABLE
.endm


.global _start
.global _start_mp

.extern loader_except
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

/*
 * 0x100: Reset
 */
_start:
    // Set up stack
    l.movhi r1, hi(_stack_top)
    l.ori   r1, r1, lo(_stack_top)
    l.addi  r1, r1, -0x10

    // Jump to loader entry
    l.jal   loader_entry
    l.nop

    // Should never reach here
    l.j     .
    l.nop

/*
 * 0x200: Bus error
 */
.align 256
    HANDLE_EXCEPT 0x2

/*
 * 0x300: Data page fault
 */
.align 256
    HANDLE_EXCEPT 0x3

/*
 * 0x400: Instr page fault
 */
.align 256
    HANDLE_EXCEPT 0x4

/*
 * 0x500: Tick timer
 */
.align 256
    HANDLE_EXCEPT 0x5

/*
 * 0x600: Alignment
 */
.align 256
    HANDLE_EXCEPT 0x6

/*
 * 0x700: Illegal instr
 */
.align 256
    HANDLE_EXCEPT 0x7

/*
 * 0x800: External interrupt
 */
.align 256
    HANDLE_EXCEPT 0x8

/*
 * 0x900: DTLB miss
 */
.align 256
    HANDLE_EXCEPT 0x9

/*
 * 0xa00: ITLB miss
 */
.align 256
    HANDLE_EXCEPT 0xa

/*
 * 0xb00: Range
 */
.align 256
    HANDLE_EXCEPT 0xb

/*
 * 0xc00: Syscall
 */
.align 256
    HANDLE_EXCEPT 0xc

/*
 * 0xd00: Floating point
 */
.align 256
    HANDLE_EXCEPT 0xd

/*
 * 0xe00: Trap
 */
.align 256
    HANDLE_EXCEPT 0xd

/*
 * 0xf00: Reserved
 */
.align 256
    HANDLE_EXCEPT 0xf


/*
 * Save registers
 */
.align 256
_save_regs_to_ctxt1:
/*     l.mtspr r0, r0, 1056    // Zero */
    l.mtspr r0, r1, 1057    // SP
    l.mtspr r0, r2, 1058
    l.mtspr r0, r3, 1059
    l.mtspr r0, r4, 1060
    l.mtspr r0, r5, 1061
    l.mtspr r0, r6, 1062
    l.mtspr r0, r7, 1063
    l.mtspr r0, r8, 1064
/*     l.mtspr r0, r9, 1065    // LR */
    l.mtspr r0, r10, 1066
    l.mtspr r0, r11, 1067
    l.mtspr r0, r12, 1068
    l.mtspr r0, r13, 1069
    l.mtspr r0, r14, 1070
    l.mtspr r0, r15, 1071
    l.mtspr r0, r16, 1072
    l.mtspr r0, r17, 1073
    l.mtspr r0, r18, 1074
    l.mtspr r0, r19, 1075
    l.mtspr r0, r20, 1076
    l.mtspr r0, r21, 1077
    l.mtspr r0, r22, 1078
    l.mtspr r0, r23, 1079
    l.mtspr r0, r24, 1080
    l.mtspr r0, r25, 1081
    l.mtspr r0, r26, 1082
    l.mtspr r0, r27, 1083
    l.mtspr r0, r28, 1084
    l.mtspr r0, r29, 1085
    l.mtspr r0, r30, 1086
    l.mtspr r0, r31, 1087
    l.jr    r9
    l.nop

.align 8
_restore_regs_from_ctxt1:
/*     l.mfspr r0, r0, 1056    // Zero */
    l.mfspr r1, r0, 1057    // SP
    l.mfspr r2, r0, 1058
    l.mfspr r3, r0, 1059
    l.mfspr r4, r0, 1060
    l.mfspr r5, r0, 1061
    l.mfspr r6, r0, 1062
    l.mfspr r7, r0, 1063
    l.mfspr r8, r0, 1064
/*     l.mfspr r9, r0, 1065    // LR */
    l.mfspr r10, r0, 1066
    l.mfspr r11, r0, 1067
    l.mfspr r12, r0, 1068
    l.mfspr r13, r0, 1069
    l.mfspr r14, r0, 1070
    l.mfspr r15, r0, 1071
    l.mfspr r16, r0, 1072
    l.mfspr r17, r0, 1073
    l.mfspr r18, r0, 1074
    l.mfspr r19, r0, 1075
    l.mfspr r20, r0, 1076
    l.mfspr r21, r0, 1077
    l.mfspr r22, r0, 1078
    l.mfspr r23, r0, 1079
    l.mfspr r24, r0, 1080
    l.mfspr r25, r0, 1081
    l.mfspr r26, r0, 1082
    l.mfspr r27, r0, 1083
    l.mfspr r28, r0, 1084
    l.mfspr r29, r0, 1085
    l.mfspr r30, r0, 1086
    l.mfspr r31, r0, 1087
    l.jr    r9
    l.nop



.align  4
_start_mp:
    // Set up stack
    l.movhi r1, hi(_stack_top_mp)
    l.ori   r1, r1, lo(_stack_top_mp)
    l.addi  r1, r1, -0x10

    // Jump to loader entry
    l.jal   loader_entry_mp
    l.nop

    // Should never reach here
    l.j     .
    l.nop
