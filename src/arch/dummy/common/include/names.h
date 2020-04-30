#ifndef __ARCH_DUMMY_COMMON_INCLUDE_NAMES_H__
#define __ARCH_DUMMY_COMMON_INCLUDE_NAMES_H__


// Arch names
#if (defined(ARCH_DUMMY))
    #define ARCH_NAME "dummy"
#else
    #error "Unknown arch type"
#endif

// Machine names
#if (defined(MACH_DUMMY))
    #define MACH_NAME "dummy"
#else
    #error "Unknown machine type"
#endif

// Model names
#if (defined(MODEL_DUMMY))
    #define MODEL_NAME "dummy"
#else
    #error "Unknown model type"
#endif


#endif
