#ifndef __ARCH_DUMMY_COMMON_INCLUDE_ARCH_H__
#define __ARCH_DUMMY_COMMON_INCLUDE_ARCH_H__


// Dummy
#if (defined(ARCH_DUMMY))

#define ARCH_NAME "dummy"
#define ARCH_WIDTH 32
#define ARCH_LITTLE_ENDIAN 1
#define ARCH_BIG_ENDIAN 0

#define HAL_BASE 0xfff88000
#define KERNEL_BASE 0xfff08000

// Unknown
#else

#error "Unknown arch type"

#endif


#endif

