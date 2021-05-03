#ifndef __ARCH_AARCH64_COMMON_INCLUDE_ABI_H__
#define __ARCH_AARCH64_COMMON_INCLUDE_ABI_H__


#define ARCH_ARM
#define ARCH_ARM64
#define ARCH_AARCH
#define ARCH_AARCH64

#define ARCH_WIDTH          64
#define ARCH_LITTLE_ENDIAN  1
#define ARCH_BIG_ENDIAN     0

#if (defined(MACH_RASPI3))
    #define LOADER_BASE     0x80000
#else
    #define LOADER_BASE     0x8000
#endif

#define HAL_BASE            0xfff88000
#define KERNEL_BASE         0xfff08000
#define USER_BASE           0x100000

#define STACK_GROWS_UP      0

#define DATA_ALIGNMENT      8


#endif
