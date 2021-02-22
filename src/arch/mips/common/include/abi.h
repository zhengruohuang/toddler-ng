#ifndef __ARCH_MIPS_COMMON_INCLUDE_ABI_H__
#define __ARCH_MIPS_COMMON_INCLUDE_ABI_H__

// Generic name
#define ARCH_MIPS

// MIPS-32 Big Endian
#if (defined(ARCH_MIPS32B))
    #define ARCH_WIDTH          32
    #define ARCH_LITTLE_ENDIAN  0
    #define ARCH_BIG_ENDIAN     1

// MIPS-32 Little Endian
#elif (defined(ARCH_MIPS32L))
    #define ARCH_WIDTH          32
    #define ARCH_LITTLE_ENDIAN  1
    #define ARCH_BIG_ENDIAN     0

// MIPS-64 Big Endian
#elif (defined(ARCH_MIPS64B))
    #define ARCH_WIDTH          64
    #define ARCH_LITTLE_ENDIAN  0
    #define ARCH_BIG_ENDIAN     1

// MIPS-64 Little Endian
#elif (defined(ARCH_MIPS64L))
    #define ARCH_WIDTH          64
    #define ARCH_LITTLE_ENDIAN  1
    #define ARCH_BIG_ENDIAN     0

// Unknown
#else
    #error "Unknown arch type"
#endif

// Base address
#if (ARCH_WIDTH == 32)
    #define LOADER_BASE     0x80010000
    #define HAL_BASE        0x80110000
    #define KERNEL_BASE     0x80182000
    #define USER_BASE       0x100000

#elif (ARCH_WIDTH == 64)
    #define LOADER_BASE     0xffffffff80010000
    #define HAL_BASE        0x9000000000110000
    #define KERNEL_BASE     0x9000000000182000
    #define USER_BASE       0x100100000

#else
    #error "Unknown arch width"
#endif

// Stack growth
#define STACK_GROWS_UP  0

// Data alignment
#define DATA_ALIGNMENT  8

#endif
