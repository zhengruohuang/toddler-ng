#include <stdio.h>
#include <stdarg.h>
#include <internal/printf.h>


/*
 * sscanf
 */
int vsscanf(const char *buf, const char *fmt, va_list args)
{
    return 0;
}

int sscanf(const char *buf, const char *fmt, ...)
{
    int count = 0;
    va_list va;
    va_start(va, fmt);

    count = vsscanf(buf, fmt, va);

    va_end(va);
    return count;
}


/*
 * fscanf
 */
int vfscanf(FILE *f, const char *fmt, va_list args)
{
    return 0;
}

int fscanf(FILE *f, const char *fmt, ...)
{
    int count = 0;
    va_list va;
    va_start(va, fmt);

    count = vfscanf(f, fmt, va);

    va_end(va);
    return count;
}


/*
 * scanf
 */
int vscanf(const char *fmt, va_list args)
{
    return 0;
}

int scanf(const char *fmt, ...)
{
    int count = 0;
    va_list va;
    va_start(va, fmt);

    count = vscanf(fmt, va);

    va_end(va);
    return count;
}
