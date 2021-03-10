#ifndef __ARCH_RISCV_COMMON_INCLUDE_ABI_H__
#define __ARCH_RISCV_COMMON_INCLUDE_ABI_H__


// RISC-V 32
#if (defined(ARCH_RISCV32))
    #define ARCH_WIDTH          32
    #define ARCH_LITTLE_ENDIAN  1
    #define ARCH_BIG_ENDIAN     0

    #define LOADER_BASE         0x80100000
    #define HAL_BASE            0xfff88000
    #define KERNEL_BASE         0xfff08000
    #define USER_BASE           0x100000

    #define STACK_GROWS_UP      0

    #define DATA_ALIGNMENT      8

// RISC-V 64
#elif (defined(ARCH_RISCV64))
    #define ARCH_WIDTH          64
    #define ARCH_LITTLE_ENDIAN  1
    #define ARCH_BIG_ENDIAN     0

    #define LOADER_BASE         0x80100000
    #define HAL_BASE            0xfff88000
    #define KERNEL_BASE         0xfff08000
    #define USER_BASE           0x100000

    #define STACK_GROWS_UP      0

    #define DATA_ALIGNMENT      8

// Unknown
#else
    #error "Unknown arch type"

#endif


#endif
