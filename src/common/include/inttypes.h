#ifndef __COMMON_INCLUDE_INTTYPES_H__
#define __COMMON_INCLUDE_INTTYPES_H__


/*
 * Fixed width data types
 */
typedef signed char         s8;
typedef signed short        s16;
typedef signed int          s32;
typedef signed long long    s64;

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;

typedef unsigned char       uchar;
typedef unsigned short      ushort;
typedef unsigned int        uint;
typedef unsigned long       ulong;
typedef unsigned long long  ulonglong;


/*
 * Generic atomic type
 */
typedef struct {
    volatile ulong value;
} atomic_t;

#define ATOMIC_ZERO { .value = 0 }


/*
 * These get redefined in libc
 */
#ifndef __USE_HOST_LIBC

// Null pointer
#ifndef NULL
#define NULL                ((void *)0)
#endif

// size_t and ssize_t
typedef unsigned long       size_t;
typedef long                ssize_t;

// Boolean
typedef unsigned int        bool;
#define true                1
#define false               0

#endif // !__USE_HOST_LIBC


#endif

