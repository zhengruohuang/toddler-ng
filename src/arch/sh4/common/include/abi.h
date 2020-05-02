#ifndef __ARCH_SH4_COMMON_INCLUDE_ABI_H__
#define __ARCH_SH4_COMMON_INCLUDE_ABI_H__


#define ARCH_WIDTH          32
#define ARCH_LITTLE_ENDIAN  1
#define ARCH_BIG_ENDIAN     0

#define LOADER_BASE         0xac800000
#define HAL_BASE            0xfff88000
#define KERNEL_BASE         0xfff08000

#define STACK_GROWS_UP      0

#define DATA_ALIGNMENT      8


#endif
