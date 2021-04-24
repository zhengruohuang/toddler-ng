#ifndef __ARCH_X86_COMMON_INCLUDE_NAMES_H__
#define __ARCH_X86_COMMON_INCLUDE_NAMES_H__


/*
 * Arch names
 */
#if (defined(ARCH_IA32))
    #define ARCH_NAME "ia32"
#elif (defined(ARCH_AMD64))
    #define ARCH_NAME "amd64"
#else
    #error "Unknown arch type"
#endif


/*
 * Machine names
 */
#if (defined(MACH_PC))
    #define MACH_NAME "pc"
#else
    #error "Unknown machine type"
#endif


/*
 * Model
 */
#if (defined(MODEL_MULTIBOOT))
    #define MODEL_NAME "multiboot"
#elif (defined(MODEL_BIOS))
    #define MODEL_NAME "bios"
#elif (defined(MODEL_UEFI))
    #define MODEL_NAME "uefi"
#elif (defined(MODEL_COREBOOT))
    #define MODEL_NAME "coreboot"
#else
    #error "Unknown model type"
#endif


#endif
