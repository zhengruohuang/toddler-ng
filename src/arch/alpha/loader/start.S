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
    br      1f
    // room for the initial PCB
    .quad   0, 0, 0, 0, 0, 0, 0, 0

    // Set up GP
1:  br      $27, 2f
2:  ldgp    $29, 0($27)

    // Set up stack
    lda     $30, _stack_top

    // Go to C!
    lda     $27, loader_entry
    jsr     $26, ($27), loader_entry

_stop:
    br .
    nop
