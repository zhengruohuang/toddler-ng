#ifndef __ARCH_DUMMY_COMMON_INCLUDE_MACH_H__
#define __ARCH_DUMMY_COMMON_INCLUDE_MACH_H__


// Dummy
#if (defined(MACH_DUMMY))
    #define MACH_NAME "dummy"

// Unknown
#else
    #error "Unknown machine type"

#endif


#endif
