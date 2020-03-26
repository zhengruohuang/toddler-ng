#ifndef __LIBK_INCLUDE_STRING_H__
#define __LIBK_INCLUDE_STRING_H__


#include "common/include/inttypes.h"


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


#endif
