#ifndef __ARCH_ALPHA_COMMON_INCLUDE_ARCH_H__
#define __ARCH_ALPHA_COMMON_INCLUDE_ARCH_H__


// Alpha
#if (defined(ARCH_ALPHA))

#define ARCH_NAME "alpha"
#define ARCH_WIDTH 64
#define ARCH_LITTLE_ENDIAN 1
#define ARCH_BIG_ENDIAN 0

// Unknown
#else

#error "Unknown arch type"

#endif


#endif
