.global _start
.global _stack_top


.section ".data.stack"

.align 16
_stack_limit:
    .fill 8192, 1, 0

.align 16
_stack_top:
    .fill 16, 1, 0


.section ".text.boot"

_start:
/*    br .
    nop*/

/*     lda $1, 0x4000($31) */
/*     addq $30, $1, $30 */

    //br .
    //nop

    //bis $31,$31,$31

    br      1f
    /* room for the initial PCB, which comes here */
    .quad   0,0,0,0,0,0,0,0
1:  br      $27, 2f
2:  ldgp    $29, 0($27)
    lda     $27, loader_entry

    br .
    nop

    // Go to C!
    jsr     $26, ($27), loader_entry

_stop:
    br .
    nop
