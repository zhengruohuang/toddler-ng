#include <stdio.h>
#include <stdarg.h>

int vprintf(const char *fmt, va_list arg)
{
    return vfprintf(stdout, fmt, arg);
}
