#include <stdio.h>
#include <stdarg.h>

int printf(const char *fmt, ...)
{
    int ret;
    va_list args;

    va_start(args, fmt);
    ret = vfprintf(stdout, fmt, args);
    va_end(args);

    return ret;
}
