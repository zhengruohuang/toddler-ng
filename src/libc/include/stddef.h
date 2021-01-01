#ifndef __LIBC_INCLUDE_STDDEF_H__
#define __LIBC_INCLUDE_STDDEF_H__


// size_t, ssize_t, and NULL
#include "common/include/inttypes.h"


#define offsetof(type, member) \
    ((ulong)&((type *)0)->member)

#define container_of(ptr, type, member) \
    ((type *)(((ulong)ptr) - (offsetof(type, member))))


typedef long ptrdiff_t;

// TODO: wchar_t


#endif
