// #ifndef __ARCH_AARCH64_COMMON_INCLUDE_ATOMIC_H__
// #define __ARCH_AARCH64_COMMON_INCLUDE_ATOMIC_H__
//
//
// /*
//  * Memory barrier
//  */
// static inline void atomic_dmb()
// {
//     // Memory barrier
//     // All memory accesses before DMB finishes before memory accesses after DMB
//     __asm__ __volatile__ ( "dmb sy;" : : : "memory" );
// }
//
// static inline void atomic_dsb()
// {
//     // Special data barrier
//     // Insturctions after DSB execute after DSB completes
//     __asm__ __volatile__ ( "dsb sy;" : : : "memory" );
// }
//
// static inline void atomic_isb()
// {
//     // Pipeline flush
//     // Instructions after ISB are fetched after ISB completes
//     __asm__ __volatile__ ( "isb;" : : : "memory" );
// }
//
// static inline void atomic_mb()
// {
//     __asm__ __volatile__ ( "dmb sy;" : : : "memory" );
// }
//
// static inline void atomic_rb()
// {
//     __asm__ __volatile__ ( "dmb sy;" : : : "memory" );
// }
//
// static inline void atomic_wb()
// {
//     __asm__ __volatile__ ( "dmb sy;" : : : "memory" );
// }
//
//
// /*
//  * Read and write
//  */
// static inline unsigned long atomic_read(volatile void *addr)
// {
//     unsigned long value = 0;
//
//     atomic_mb();
//     value = *(volatile unsigned long *)addr;
//     atomic_mb();
//
//     return value;
// }
//
// static inline void atomic_write(volatile void *addr, unsigned long value)
// {
//     atomic_mb();
//     *(volatile unsigned long *)addr = value;
//     atomic_mb();
// }
//
//
// /*
//  * Compare and swap
//  */
// static inline int atomic_cas_bool(volatile void *addr,
//     unsigned long old_val, unsigned long new_val)
// {
//     unsigned long failed, read;
//
//     __asm__ __volatile__ (
//         "1: ldxr %[read], %[ptr];"
//         "   cmp %[read], %[val_old];"
//         "   b.ne 2f;"
//         "   stxr %[failed], %[val_new], %[ptr];"
//         "   cbnz %[failed], 1b;"
//         "2:;"
//         : [failed] "=&r" (failed), [read] "=&r" (read)
//         : [ptr] "r" (addr), [val_old] "r" (old_val), [val_new] "r" (new_val)
//         : "cc", "memory"
//     );
//
//     return read == old_val;
// }
//
//
// /*
//  * Fetch and add
//  */
// static inline void atomic_inc(volatile unsigned long *addr)
// {
//     unsigned long tmp, failed;
//
//     __asm__ __volatile__ (
//         "1: ldxr %[tmp], %[ptr];"
//         "   add %x[tmp], %x[tmp], #1;"
//         "   stxr %[failed], %[tmp], %[ptr];"
//         "   cbnz %[failed], 1b;"
//         : [tmp] "=&r" (tmp), [failed] "=&r" (failed)
//         : [ptr] "r" (addr)
//         : "memory"
//     );
// }
//
// static inline void atomic_dec(volatile unsigned long *addr)
// {
//     unsigned long tmp, failed;
//
//     __asm__ __volatile__ (
//         "1: ldxr %[tmp], %[ptr];"
//         "   dec %x[tmp], %x[tmp], #1;"
//         "   stxr %[failed], %[tmp], %[ptr];"
//         "   cbnz %[failed], 1b;"
//         : [tmp] "=&r" (tmp), [failed] "=&r" (failed)
//         : [ptr] "r" (addr)
//         : "memory"
//     );
// }
//
//
// #endif
