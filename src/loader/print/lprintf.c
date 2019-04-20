#include "common/include/stdarg.h"
#include "loader/include/lprintf.h"


int lprintf(const char *fmt, ...)
{
    int ret = 0;

    va_list va;
    va_start(va, fmt);

    ret = vlprintf(fmt, &va);

    va_end(va);

    return ret;
}
