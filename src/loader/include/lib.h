#ifndef __LOADER_INCLUDE_LIB_H__
#define __LOADER_INCLUDE_LIB_H__


#include "common/include/inttypes.h"


/*
 * Alignment
 */
#define ALIGN_UP(s, a)      (((s) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(s, a)    ((s) & ~((a) - 1))


/*
 * Debug
 */
#define stop()              for (;;)
#define abort()             __abort(__FILE__, __LINE__)
#define warn(...)           __warn((void *)0, __FILE__, __LINE__, __VA_ARGS__)
#define warn_if(ex, ...)    (void)(!(ex) || (__warn("!(" #ex ")", __FILE__, __LINE__, __VA_ARGS__), 0))
#define warn_not(ex, ...)   (void)((ex) || (__warn(#ex, __FILE__, __LINE__, __VA_ARGS__), 0))
#define panic(...)          __panic((void *)0, __FILE__, __LINE__, __VA_ARGS__)
#define panic_if(ex, ...)   (void)(!(ex) || (__panic("!(" #ex ")", __FILE__, __LINE__, __VA_ARGS__), 0))
#define panic_not(ex, ...)  (void)((ex) || (__panic(#ex, __FILE__, __LINE__, __VA_ARGS__), 0))
#define assert(ex)          (void)((ex) || (__assert(#ex, __FILE__, __LINE__, ""), 0))

extern void __abort(const char *file, int line);
extern void __warn(const char *ex, const char *file, int line, const char *msg, ...);
extern void __panic(const char *ex, const char *file, int line, const char *msg, ...);
extern void __assert(const char *ex, const char *file, int line, const char *msg, ...);


/*
 * Bit
 */
extern u16 swap_endian16(u16 val);
extern u32 swap_endian32(u32 val);
extern u64 swap_endian64(u64 val);
extern ulong swap_endian(ulong val);
extern u16 swap_big_endian16(u16 val);
extern u32 swap_big_endian32(u32 val);
extern u64 swap_big_endian64(u64 val);
extern ulong swap_big_endian(ulong val);
extern u16 swap_little_endian16(u16 val);
extern u32 swap_little_endian32(u32 val);
extern u64 swap_little_endian64(u64 val);
extern ulong swap_little_endian(ulong val);

extern int popcount64(u64 val);
extern int popcount32(u32 val);
extern int popcount16(u16 val);
extern int popcount(ulong val);

extern int clz64(u64 val);
extern int clz32(u32 val);
extern int clz16(u16 val);
extern int clz(ulong val);


/*
 * String & Memory
 */
extern char *strcpy(char *dest, const char *src);
extern char *strncpy(char *dest, const char *src, size_t n);
extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern size_t strlen(const char *s);
extern char *strpbrk(const char *str1, const char *str2);
extern char *strchr(const char *str, int c);
extern char *strstr(const char *str1, const char *str2);

extern void *memcpy(void *dest, const void *src, size_t count);
extern void *memset(void *src, int value, size_t size);
extern void *memzero(void *src, size_t size);
extern int memcmp(const void *src1, const void *src2, size_t len);
extern void *memchr(const void *blk, int c, size_t size);
extern void *memstr(const void *blk, const char *needle, size_t size);

extern int isdigit(int ch);
extern int isxdigit(int ch);

extern u64 stou64(const char *str, int base);
extern u32 stou32(const char *str, int base);
extern ulong stoul(const char *str, int base);
extern int stoi(const char *str, int base);
extern int atoi(const char *str);


/*
 * Math
 */
extern u32 mul_u32(u32 a, u32 b);
extern s32 mul_s32(s32 a, s32 b);
extern u64 mul_u64(u64 a, u64 b);
extern s64 mul_s64(s64 a, s64 b);

extern void div_u32(u32 a, u32 b, u32 *qout, u32 *rout);
extern void div_s32(s32 a, s32 b, s32 *qout, s32 *rout);
extern void div_u64(u64 a, u64 b, u64 *qout, u64 *rout);
extern void div_s64(s64 a, s64 b, s64 *qout, s64 *rout);


#endif
