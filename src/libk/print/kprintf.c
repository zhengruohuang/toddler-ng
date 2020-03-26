#include "common/include/stdarg.h"
#include "libk/include/kprintf.h"


int __kprintf(const char *fmt, ...)
{
    int ret = 0;

    va_list va;
    va_start(va, fmt);

    ret = __vkprintf(fmt, va);

    va_end(va);

    return ret;
}
