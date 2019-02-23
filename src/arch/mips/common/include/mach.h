#ifndef __ARCH_MIPS_COMMON_INCLUDE_MACH_H__
#define __ARCH_MIPS_COMMON_INCLUDE_MACH_H__


// Malta
#if (defined(MACH_MALTA))

#define MACH_NAME "malta"

// Unknown
#else

#error "Unknown machine type"

#endif


#endif
