#ifndef __ARCH_M68K_COMMON_INCLUDE_ARCH_H__
#define __ARCH_M68K_COMMON_INCLUDE_ARCH_H__


// Dummy
#if (defined(ARCH_M68K))

#define ARCH_NAME "m68k"
#define ARCH_WIDTH 32
#define ARCH_LITTLE_ENDIAN 0
#define ARCH_BIG_ENDIAN 1

// Unknown
#else

#error "Unknown arch type"

#endif


#endif

