#ifndef __ARCH_AARCH64_COMMON_INCLUDE_ARCH_H__
#define __ARCH_AARCH64_COMMON_INCLUDE_ARCH_H__


// AArch64v8
#if (defined(ARCH_AARCH64V8))

#define ARCH_NAME "aarch64v8"
#define ARCH_WIDTH 64
#define ARCH_LITTLE_ENDIAN 1
#define ARCH_BIG_ENDIAN 0

// Unknown
#else

#error "Unknown arch type"

#endif


#endif

