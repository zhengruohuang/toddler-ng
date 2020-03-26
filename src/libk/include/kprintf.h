#ifndef __LIBK_INCLUDE_KPRINTF_H__
#define __LIBK_INCLUDE_KPRINTF_H__


#include "common/include/stdarg.h"


typedef int(*periph_putchar_t)(int ch);

extern void init_libk_putchar(periph_putchar_t func);

extern int __kputchar(int ch);
extern int __vkprintf(const char *fmt, va_list arg);
extern int __kprintf(const char *fmt, ...);


#endif
