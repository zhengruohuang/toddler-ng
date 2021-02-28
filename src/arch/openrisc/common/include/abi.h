#ifndef __ARCH_OPENRISC_COMMON_INCLUDE_ABI_H__
#define __ARCH_OPENRISC_COMMON_INCLUDE_ABI_H__

#define ARCH_WIDTH          32
#define ARCH_LITTLE_ENDIAN  0
#define ARCH_BIG_ENDIAN     1

#define LOADER_BASE         0x100
#define HAL_BASE            0x1088000
#define KERNEL_BASE         0x1008000
#define USER_BASE           0x10000000

#define STACK_GROWS_UP      0

#define DATA_ALIGNMENT      8

#endif
