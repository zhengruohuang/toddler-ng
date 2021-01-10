#ifndef __LIBC_INCLUDE_CRT_H__
#define __LIBC_INCLUDE_CRT_H__

#include <stddef.h>

struct crt_entry_param {
    int argc;
    size_t argv_size;
    char argv[];
};

#endif
