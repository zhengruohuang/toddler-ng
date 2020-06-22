.global _start

.section ".text.boot"

_start:
    br      1f

    // Set up GP
1:  br      $27, 2f
2:  ldgp    $29, 0($27)

    // Go to C!
    lda     $27, hal_entry
    jsr     $26, ($27), hal_entry

_stop:
    br .
    nop
