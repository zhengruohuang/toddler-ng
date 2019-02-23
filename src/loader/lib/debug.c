#include "common/include/stdarg.h"
#include "loader/include/lprintf.h"
#include "loader/include/lib.h"


void __abort(const char *file, int line)
{
    lprintf("!!! Abort !!!\nFile: %s, line %d\n", file, line);
    stop();
}

void __warn(const char *ex, const char *file, int line, const char *msg, ...)
{
    va_list va;
    va_start(va, msg);

    if (ex) {
        lprintf("!!! Warn: %s !!!\nFile: %s, line: %d\n", ex, file, line);
    } else {
        lprintf("!!! Warn !!!\nFile: %s, line: %d\n", file, line);
    }
    vlprintf(msg, va);
    lprintf("\n");

    va_end(va);
}

void __panic(const char *ex, const char *file, int line, const char *msg, ...)
{
    va_list va;
    va_start(va, msg);

    if (ex) {
        lprintf("!!! Panic: %s !!!\nFile: %s, line: %d\n", ex, file, line);
    } else {
        lprintf("!!! Panic !!!\nFile: %s, line %d\n", file, line);
    }
    vlprintf(msg, va);
    lprintf("\n");

    va_end(va);
    stop();
}

void __assert(const char *ex, const char *file, int line, const char *msg, ...)
{
    va_list va;
    va_start(va, msg);

    lprintf("!!! Assert failed: %s !!!\nFile: %s, line: %d\n", ex, file, line);
    vlprintf(msg, va);
    lprintf("\n");

    va_end(va);
    stop();
}
