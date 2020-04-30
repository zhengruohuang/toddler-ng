#ifndef __ARCH_OPENRISC_COMMON_INCLUDE_NAMES_H__
#define __ARCH_OPENRISC_COMMON_INCLUDE_NAMES_H__


// Arch names
#if (defined(ARCH_OPENRISC))
    #define ARCH_NAME "openrisc"
#else
    #error "Unknown arch type"
#endif

// Machine names
#if (defined(MACH_SIM))
    #define MACH_NAME "sim"
#else
    #error "Unknown machine type"
#endif

// Model names
#if (defined(MODEL_QEMU))
    #define MODEL_NAME "qemu"
#else
    #error "Unknown model type"
#endif


#endif
