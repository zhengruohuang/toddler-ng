#ifndef __LIBC_INCLUDE_STDARG_H__
#define __LIBC_INCLUDE_STDARG_H__


#include "common/include/stdarg.h"


#define va_copy(d, s)   __builtin_va_copy(d, s)
#define __va_copy(d,s)  __builtin_va_copy(d,s)


#endif
