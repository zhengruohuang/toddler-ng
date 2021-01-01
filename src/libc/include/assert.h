#ifndef __LIBC_INCLUDE_ASSERT_H__
#define __LIBC_INCLUDE_ASSERT_H__


#include "libk/include/debug.h"


#ifdef NDEBUG
#ifdef assert
#undef assert
#endif
#define assert(cond) ((void)0)
#endif


#endif
