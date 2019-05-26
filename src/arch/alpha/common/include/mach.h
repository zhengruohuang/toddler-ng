#ifndef __ARCH_ALPHA_COMMON_INCLUDE_MACH_H__
#define __ARCH_ALPHA_COMMON_INCLUDE_MACH_H__


// Clipper
#if (defined(MACH_CLIPPER))
    #define MACH_NAME "clipper"
    #define LOADER_BASE_LD 0xfffffc0001010000
    #define LOADER_BASE 0xfffffc0001010000ull

// Unknown
#else
    #error "Unknown machine type"

#endif


#endif
