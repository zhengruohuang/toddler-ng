#include <stdio.h>
#include <stdarg.h>

int fprintf(FILE *f, const char *fmt, ...)
{
    int ret;
    va_list args;

    va_start(args, fmt);
    ret = vfprintf(f, fmt, args);
    va_end(args);

    return ret;
}
