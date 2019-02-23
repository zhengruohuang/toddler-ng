#ifndef __COMMON_INCLUDE_INTTYPES_H__
#define __COMMON_INCLUDE_INTTYPES_H__


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


#ifndef NULL
#define NULL                ((void *)0)
#endif


#ifndef AVOID_LIBC_CONFLICT

typedef unsigned long       size_t;

typedef unsigned int        bool;
#define true                1
#define false               0

#endif


#endif

