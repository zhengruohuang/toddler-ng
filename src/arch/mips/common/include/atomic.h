#ifndef __ARCH_MIPS_COMMON_INCLUDE_ATOMIC__
#define __ARCH_MIPS_COMMON_INCLUDE_ATOMIC__


/*
 * Memory barrier
 */
static inline void atomic_mb()
{
    __asm__ __volatile__ ( "sync;" : : : "memory" );
}

static inline void atomic_rb()
{
    __asm__ __volatile__ ( "sync;" : : : "memory" );
}

static inline void atomic_wb()
{
    __asm__ __volatile__ ( "sync;" : : : "memory" );
}


/*
 * Read and write
 */
static inline unsigned long atomic_read(volatile void *addr)
{
    unsigned long value = 0;

    atomic_mb();
    value = *(volatile unsigned long *)addr;
    atomic_mb();

    return value;
}

static inline void atomic_write(volatile void *addr, unsigned long value)
{
    atomic_mb();
    *(volatile unsigned long *)addr = value;
    atomic_mb();
}


/*
 * Compare and swap
 */
static inline int atomic_cas(volatile void *addr,
    unsigned long old_val, unsigned long new_val)
{
    register unsigned long tmp, read;

    __asm__ __volatile__ (
        "1: sync;"
        "   ll %[read], 0(%[ptr]);"
        "   bne %[read], %[val_old], 2f;"
        "   nop;"
        "   move %[tmp], %[val_new];"
        "   sc %[tmp], 0(%[ptr]);"
        "   beq %[tmp], %[zero], 1b;"
        "   nop;"
        "2: sync;"
        : [tmp] "=&r" (tmp), [read] "=&r" (read)
        : [ptr] "r" (addr), [zero] "i" (0),
            [val_old] "r" (old_val), [val_new] "r" (new_val)
        : "memory"
    );

    return read == old_val;
}


/*
 * Fetch and add
 */
static inline void atomic_inc(volatile unsigned long *addr)
{
    register unsigned long tmp;

    __asm__ __volatile__ (
        "1: sync;"
        "   ll %[tmp], 0(%[ptr]);"
        "   addi %[tmp], %[tmp], 0x1;"
        "   sc %[tmp], 0(%[ptr]);"
        "   beq %[tmp], %[zero], 1b;"
        "   nop;"
        "2: sync;"
        : [tmp] "=&r" (tmp)
        : [ptr] "r" (addr), [zero] "i" (0)
        : "memory"
    );
}

static inline void atomic_dec(volatile unsigned long *addr)
{
    register unsigned long tmp;

    __asm__ __volatile__ (
        "1: sync;"
        "   ll %[tmp], 0(%[ptr]);"
        "   addi %[tmp], %[tmp], -0x1;"
        "   sc %[tmp], 0(%[ptr]);"
        "   beq %[tmp], %[zero], 1b;"
        "   nop;"
        "2: sync;"
        : [tmp] "=&r" (tmp)
        : [ptr] "r" (addr), [zero] "i" (0)
        : "memory"
    );
}


#endif
