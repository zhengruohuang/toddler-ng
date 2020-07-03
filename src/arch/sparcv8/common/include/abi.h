#ifndef __ARCH_SPARCV8_COMMON_INCLUDE_ABI_H__
#define __ARCH_SPARCV8_COMMON_INCLUDE_ABI_H__


#define ARCH_WIDTH          32
#define ARCH_LITTLE_ENDIAN  0
#define ARCH_BIG_ENDIAN     1

#if (defined(MACH_LEON3))
#define LOADER_BASE         0x40001000
#elif (defined(MACH_SUN4M))
#define LOADER_BASE         0x4000
#else
#define LOADER_BASE         0x4000
#endif

#define HAL_BASE            0xfff88000
#define KERNEL_BASE         0xfff08000

#define STACK_GROWS_UP      0

#define DATA_ALIGNMENT      8
#define POSITION_INDEP      0


#endif
