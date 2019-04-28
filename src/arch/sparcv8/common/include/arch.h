#ifndef __ARCH_SPARCV8_COMMON_INCLUDE_ARCH_H__
#define __ARCH_SPARCV8_COMMON_INCLUDE_ARCH_H__


// SPARCv8
#if (defined(ARCH_SPARCV8))

#define ARCH_NAME "sparcv8"
#define ARCH_WIDTH 32
#define ARCH_LITTLE_ENDIAN 0
#define ARCH_BIG_ENDIAN 1

// Unknown
#else

#error "Unknown arch type"

#endif


#endif
