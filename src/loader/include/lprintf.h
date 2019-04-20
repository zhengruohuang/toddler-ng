#ifndef __LOADER_INCLUDE_LPRINTF_H__
#define __LOADER_INCLUDE_LPRINTF_H__


#include "common/include/stdarg.h"


typedef int(*periph_putchar_t)(int ch);

extern void init_putchar(periph_putchar_t func);
extern int lputchar(int ch);
extern int vlprintf(const char *fmt, va_list *arg);
extern int lprintf(const char *fmt, ...);


#endif
