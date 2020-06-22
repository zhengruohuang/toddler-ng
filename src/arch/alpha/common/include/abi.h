#ifndef __ARCH_ALPHA_COMMON_INCLUDE_ABI_H__
#define __ARCH_ALPHA_COMMON_INCLUDE_ABI_H__


#define ARCH_WIDTH          64
#define ARCH_LITTLE_ENDIAN  1
#define ARCH_BIG_ENDIAN     0

#define LOADER_BASE         0xfffffc0001010000
#define HAL_BASE            0xfffffc0002000000
#define KERNEL_BASE         0xfffffc0002400000

#define STACK_GROWS_UP      0

#define DATA_ALIGNMENT      8


#endif
