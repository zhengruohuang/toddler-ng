#include "common/include/abi.h"


#if (ARCH_WIDTH == 32)
    #define WORD_SIZE       4
    #define REG_BASE_OFFSET 8
    #define st_ul           sw
#elif (ARCH_WIDTH == 64)
    #define WORD_SIZE       8
    #define REG_BASE_OFFSET 16
    #define st_ul           sd
#endif


.extern int_handler_entry

.global raw_int_entry


.balign 16
raw_int_entry:
    /* Swap SP and sscratch */
    csrrw   x2, sscratch, x2

    /* Save all registers */
 /* st_ul   x0,  (REG_BASE_OFFSET + WORD_SIZE * 0)(x2) */ /* Zero */
    st_ul   x1,  (REG_BASE_OFFSET + WORD_SIZE * 1)(x2)
 /* st_ul   x2,  (REG_BASE_OFFSET + WORD_SIZE * 2)(x2) */ /* SP */
    st_ul   x3,  (REG_BASE_OFFSET + WORD_SIZE * 3)(x2)
    st_ul   x4,  (REG_BASE_OFFSET + WORD_SIZE * 4)(x2)
    st_ul   x5,  (REG_BASE_OFFSET + WORD_SIZE * 5)(x2)
    st_ul   x6,  (REG_BASE_OFFSET + WORD_SIZE * 6)(x2)
    st_ul   x7,  (REG_BASE_OFFSET + WORD_SIZE * 7)(x2)
    st_ul   x8,  (REG_BASE_OFFSET + WORD_SIZE * 8)(x2)
    st_ul   x9,  (REG_BASE_OFFSET + WORD_SIZE * 9)(x2)

    st_ul   x10, (REG_BASE_OFFSET + WORD_SIZE * 10)(x2)
    st_ul   x11, (REG_BASE_OFFSET + WORD_SIZE * 11)(x2)
    st_ul   x12, (REG_BASE_OFFSET + WORD_SIZE * 12)(x2)
    st_ul   x13, (REG_BASE_OFFSET + WORD_SIZE * 13)(x2)
    st_ul   x14, (REG_BASE_OFFSET + WORD_SIZE * 14)(x2)
    st_ul   x15, (REG_BASE_OFFSET + WORD_SIZE * 15)(x2)
    st_ul   x16, (REG_BASE_OFFSET + WORD_SIZE * 16)(x2)
    st_ul   x17, (REG_BASE_OFFSET + WORD_SIZE * 17)(x2)
    st_ul   x18, (REG_BASE_OFFSET + WORD_SIZE * 18)(x2)
    st_ul   x19, (REG_BASE_OFFSET + WORD_SIZE * 19)(x2)

    st_ul   x20, (REG_BASE_OFFSET + WORD_SIZE * 20)(x2)
    st_ul   x21, (REG_BASE_OFFSET + WORD_SIZE * 21)(x2)
    st_ul   x22, (REG_BASE_OFFSET + WORD_SIZE * 22)(x2)
    st_ul   x23, (REG_BASE_OFFSET + WORD_SIZE * 23)(x2)
    st_ul   x24, (REG_BASE_OFFSET + WORD_SIZE * 24)(x2)
    st_ul   x25, (REG_BASE_OFFSET + WORD_SIZE * 25)(x2)
    st_ul   x26, (REG_BASE_OFFSET + WORD_SIZE * 26)(x2)
    st_ul   x27, (REG_BASE_OFFSET + WORD_SIZE * 27)(x2)
    st_ul   x28, (REG_BASE_OFFSET + WORD_SIZE * 28)(x2)
    st_ul   x29, (REG_BASE_OFFSET + WORD_SIZE * 29)(x2)

    st_ul   x30, (REG_BASE_OFFSET + WORD_SIZE * 30)(x2)
    st_ul   x31, (REG_BASE_OFFSET + WORD_SIZE * 31)(x2)

    /* Prepare C arg */
    mv      x10, x2

    /* Save original SP */
    csrrw   x2, sscratch, x2
    st_ul   x2, (REG_BASE_OFFSET + WORD_SIZE * 2)(x10)

    /* Set up int handler SP */
    mv      x2, x10
    addi    x2, x2, -16

    /* Go to C handler */
    jal     int_handler_entry
    nop

    /* Should never reach here */
    j       .
    nop
