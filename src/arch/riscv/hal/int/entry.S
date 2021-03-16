#define WORD_SIZE       4
#define REG_BASE_OFFSET 8


.extern int_handler_entry

.global raw_int_entry


.balign 16
raw_int_entry:
    /* Swap SP and sscratch */
    csrrw   x2, sscratch, x2

    /* Save all registers */
 /* sw      x0,  (REG_BASE_OFFSET + WORD_SIZE * 0)(x2) */ /* Zero */
    sw      x1,  (REG_BASE_OFFSET + WORD_SIZE * 1)(x2)
 /* sw      x2,  (REG_BASE_OFFSET + WORD_SIZE * 2)(x2) */ /* SP */
    sw      x3,  (REG_BASE_OFFSET + WORD_SIZE * 3)(x2)
    sw      x4,  (REG_BASE_OFFSET + WORD_SIZE * 4)(x2)
    sw      x5,  (REG_BASE_OFFSET + WORD_SIZE * 5)(x2)
    sw      x6,  (REG_BASE_OFFSET + WORD_SIZE * 6)(x2)
    sw      x7,  (REG_BASE_OFFSET + WORD_SIZE * 7)(x2)
    sw      x8,  (REG_BASE_OFFSET + WORD_SIZE * 8)(x2)
    sw      x9,  (REG_BASE_OFFSET + WORD_SIZE * 9)(x2)

    sw      x10, (REG_BASE_OFFSET + WORD_SIZE * 10)(x2)
    sw      x11, (REG_BASE_OFFSET + WORD_SIZE * 11)(x2)
    sw      x12, (REG_BASE_OFFSET + WORD_SIZE * 12)(x2)
    sw      x13, (REG_BASE_OFFSET + WORD_SIZE * 13)(x2)
    sw      x14, (REG_BASE_OFFSET + WORD_SIZE * 14)(x2)
    sw      x15, (REG_BASE_OFFSET + WORD_SIZE * 15)(x2)
    sw      x16, (REG_BASE_OFFSET + WORD_SIZE * 16)(x2)
    sw      x17, (REG_BASE_OFFSET + WORD_SIZE * 17)(x2)
    sw      x18, (REG_BASE_OFFSET + WORD_SIZE * 18)(x2)
    sw      x19, (REG_BASE_OFFSET + WORD_SIZE * 19)(x2)

    sw      x20, (REG_BASE_OFFSET + WORD_SIZE * 20)(x2)
    sw      x21, (REG_BASE_OFFSET + WORD_SIZE * 21)(x2)
    sw      x22, (REG_BASE_OFFSET + WORD_SIZE * 22)(x2)
    sw      x23, (REG_BASE_OFFSET + WORD_SIZE * 23)(x2)
    sw      x24, (REG_BASE_OFFSET + WORD_SIZE * 24)(x2)
    sw      x25, (REG_BASE_OFFSET + WORD_SIZE * 25)(x2)
    sw      x26, (REG_BASE_OFFSET + WORD_SIZE * 26)(x2)
    sw      x27, (REG_BASE_OFFSET + WORD_SIZE * 27)(x2)
    sw      x28, (REG_BASE_OFFSET + WORD_SIZE * 28)(x2)
    sw      x29, (REG_BASE_OFFSET + WORD_SIZE * 29)(x2)

    sw      x30, (REG_BASE_OFFSET + WORD_SIZE * 30)(x2)
    sw      x31, (REG_BASE_OFFSET + WORD_SIZE * 31)(x2)

    /* Prepare C arg */
    mv      x10, x2

    /* Save original SP */
    csrrw   x2, sscratch, x2
    sw      x2, (REG_BASE_OFFSET + WORD_SIZE * 2)(x10)

    /* Set up int handler SP */
    mv      x2, x10
    addi    x2, x2, -16

    /* Go to C handler */
    jal     int_handler_entry
    nop

    /* Should never reach here */
    j       .
    nop
