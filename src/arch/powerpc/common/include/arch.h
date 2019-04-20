#ifndef __ARCH_POWERPC_COMMON_INCLUDE_ARCH_H__
#define __ARCH_POWERPC_COMMON_INCLUDE_ARCH_H__


// PowerPC 32-bit
#if (defined(ARCH_POWERPC))

#define ARCH_NAME "powerpc"
#define ARCH_WIDTH 32
#define ARCH_LITTLE_ENDIAN 0
#define ARCH_BIG_ENDIAN 1

// Unknown
#else

#error "Unknown arch type"

#endif


#endif

