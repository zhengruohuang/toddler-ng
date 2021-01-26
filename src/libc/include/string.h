#ifndef __LIBC_INCLUDE_STRING_H__
#define __LIBC_INCLUDE_STRING_H__

#include "common/include/inttypes.h"
#include "libk/include/string.h"

extern char *strdup(const char *s);
extern char *strcat(char *dest, const char *src);
extern char *strncat(char *dest, const char *src, size_t n);
extern char *strcatcpy(const char *src1, const char *src2);

#endif
