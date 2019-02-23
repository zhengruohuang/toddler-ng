#ifndef __ARCH_ARM_COMMON_INCLUDE_ARCH_H__
#define __ARCH_ARM_COMMON_INCLUDE_ARCH_H__


// ARMv7
#if (defined(ARCH_ARMV7))

#define ARCH_NAME "armv7"
#define ARCH_WIDTH 32
#define ARCH_LITTLE_ENDIAN 1
#define ARCH_BIG_ENDIAN 0

// Unknown
#else

#error "Unknown arch type"

#endif


#endif

