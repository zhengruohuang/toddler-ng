#ifndef __ARCH_X86_COMMON_INCLUDE_ABI_H__
#define __ARCH_X86_COMMON_INCLUDE_ABI_H__


// Generic name
#define ARCH_X86


// IA32
#if (defined(ARCH_IA32))
    #define ARCH_WIDTH          32
    #define ARCH_LITTLE_ENDIAN  1
    #define ARCH_BIG_ENDIAN     0

    #define LOADER_BASE         0x200000
    #define HAL_BASE            0xfff88000
    #define KERNEL_BASE         0xfff08000
    #define USER_BASE           0x100000

    #define STACK_GROWS_UP      0

    #define DATA_ALIGNMENT      8

// AMD64
#elif (defined(ARCH_AMD64))
    #define ARCH_WIDTH          64
    #define ARCH_LITTLE_ENDIAN  1
    #define ARCH_BIG_ENDIAN     0

    #define LOADER_BASE         0x200000
    #define HAL_BASE            0xffffffffc8800000
    #define KERNEL_BASE         0xffffffffc0800000
    #define USER_BASE           0x100100000 // 4GB + 1MB

    #define STACK_GROWS_UP      0

    #define DATA_ALIGNMENT      8

// Unknown
#else
    #error "Unknown arch type"

#endif


#endif
