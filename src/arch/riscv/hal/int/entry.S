.extern int_handler_entry

.global raw_int_entry


.balign 16
raw_int_entry:
    /* Swap SP and sscratch */
    csrrw   x2, sscratch, x2

    /* Save all registers */
 /* sw      x0,    0(x2) */ /* Zero */
    sw      x1,    4(x2)
 /* sw      x2,    8(x2) */ /* SP */
    sw      x3,   12(x2)
    sw      x4,   16(x2)
    sw      x5,   20(x2)
    sw      x6,   24(x2)
    sw      x7,   28(x2)
    sw      x8,   32(x2)
    sw      x9,   36(x2)

    sw      x10,  40(x2)
    sw      x11,  44(x2)
    sw      x12,  48(x2)
    sw      x13,  52(x2)
    sw      x14,  56(x2)
    sw      x15,  60(x2)
    sw      x16,  64(x2)
    sw      x17,  68(x2)
    sw      x18,  72(x2)
    sw      x19,  76(x2)

    sw      x20,  80(x2)
    sw      x21,  84(x2)
    sw      x22,  88(x2)
    sw      x23,  92(x2)
    sw      x24,  96(x2)
    sw      x25, 100(x2)
    sw      x26, 104(x2)
    sw      x27, 108(x2)
    sw      x28, 112(x2)
    sw      x29, 116(x2)

    sw      x30, 120(x2)
    sw      x31, 124(x2)

    /* Prepare C arg */
    mv      x10, x2

    /* Save original SP */
    csrrw   x2, sscratch, x2
    sw      x2, 8(x10)

    /* Set up int handler SP */
    mv      x2, x10

    /* Go to C handler */
    jal     int_handler_entry
    nop

    /* Should never reach here */
    j       .
    nop
