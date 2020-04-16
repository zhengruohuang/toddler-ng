#ifndef __ARCH_ARM_COMMON_INCLUDE_ATOMIC_H__
#define __ARCH_ARM_COMMON_INCLUDE_ATOMIC_H__


/*
 * Pause
 */
static inline void atomic_pause()
{
    // Wait for event
    __asm__ __volatile__ ( "wfe" : : : "memory" );
}

static inline void atomic_notify()
{
    // Send event
    __asm__ __volatile__ ( "sev" : : : "memory" );
}


/*
 * Memory barrier
 */
static inline void atomic_dmb()
{
    // Memory barrier
    // All memory accesses before DMB finishes before memory accesses after DMB
    __asm__ __volatile__ ( "dmb;" : : : "memory" );
}

static inline void atomic_dsb()
{
    // Special data barrier
    // Insturctions after DSB execute after DSB completes
    __asm__ __volatile__ ( "dsb;" : : : "memory" );
}

static inline void atomic_isb()
{
    // Pipeline flush
    // Instructions after ISB are fetched after ISB completes
    __asm__ __volatile__ ( "isb;" : : : "memory" );
}

static inline void atomic_mb()
{
    __asm__ __volatile__ ( "dmb;" : : : "memory" );
}

static inline void atomic_rb()
{
    __asm__ __volatile__ ( "dmb;" : : : "memory" );
}

static inline void atomic_wb()
{
    __asm__ __volatile__ ( "dmb;" : : : "memory" );
}


/*
 * Read and write
 */
static inline ulong atomic_read(volatile void *addr)
{
    ulong value = 0;

    atomic_mb();
    value = *(volatile ulong *)addr;
    atomic_mb();

    return value;
}

static inline void atomic_write(volatile ulong *addr, ulong value)
{
    atomic_mb();
    *(volatile ulong *)addr = value;
    atomic_mb();
}


/*
 * Compare and swap
 */
static inline int atomic_cas(volatile ulong *addr,
                             ulong old_val, ulong new_val)
{
    ulong failed, read;

    __asm__ __volatile__ (
        "1: ldrex %[read], [%[ptr], #0];"
        "   cmp %[read], %[val_old];"
        "   bne 2f;"
        "   strex %[failed], %[val_new], [%[ptr], #0];"
        "   cmp %[failed], #1;"
        "   beq 1b;"
        "2:;"
        : [failed] "=&r" (failed), [read] "=&r" (read)
        : [ptr] "r" (addr), [val_old] "r" (old_val), [val_new] "r" (new_val)
        : "cc", "memory"
    );

    return read == old_val;
}

static inline ulong atomic_cas_val(volatile ulong *addr,
                                   ulong old_val, ulong new_val)
{
//     return __sync_val_compare_and_swap(addr, old_val, new_val);

    ulong failed, read;

    __asm__ __volatile__ (
        "1: ldrex %[read], [%[ptr], #0];"
        "   cmp %[read], %[val_old];"
        "   bne 2f;"
        "   strex %[failed], %[val_new], [%[ptr], #0];"
        "   cmp %[failed], #1;"
        "   beq 1b;"
        "2:;"
        : [failed] "=&r" (failed), [read] "=&r" (read)
        : [ptr] "r" (addr), [val_old] "r" (old_val), [val_new] "r" (new_val)
        : "cc", "memory"
    );

    return read;
}


/*
 * Fetch and add
 */
static inline void atomic_inc(volatile ulong *addr)
{
//     __sync_fetch_and_add(addr, 1);

    ulong tmp, failed;

    __asm__ __volatile__ (
        "1: ldrex %[tmp], [%[ptr], #0];"
        "   add %[tmp], %[tmp], #1;"
        "   strex %[failed], %[tmp], [%[ptr], #0];"
        "   cmp %[failed], #1;"
        "   beq 1b;"
        : [tmp] "=&r" (tmp), [failed] "=&r" (failed)
        : [ptr] "r" (addr)
        : "cc", "memory"
    );
}

static inline void atomic_dec(volatile ulong *addr)
{
//     __sync_fetch_and_sub(addr, 1);

    ulong tmp, failed;

    __asm__ __volatile__ (
        "1: ldrex %[tmp], [%[ptr], #0];"
        "   sub %[tmp], %[tmp], #1;"
        "   strex %[failed], %[tmp], [%[ptr], #0];"
        "   cmp %[failed], #1;"
        "   beq 1b;"
        : [tmp] "=&r" (tmp), [failed] "=&r" (failed)
        : [ptr] "r" (addr)
        : "cc", "memory"
    );
}


#endif
