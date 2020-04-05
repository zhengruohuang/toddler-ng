#include "common/include/inttypes.h"
#include "common/include/stdarg.h"
#include "libk/include/debug.h"
#include "libk/include/kprintf.h"


static cpu_stop_t stop_func = NULL;


void init_libk_stop(cpu_stop_t func)
{
    stop_func = func;
}

void __stop()
{
    if (stop_func) {
        stop_func();
    }

    while (1);
}

// void __abort(const char *file, int line)
// {
//     __kprintf("!!! ABORT !!!\nFile: %s, line %d\n", file, line);
//     __stop();
// }
//
// void __warn(const char *ex, const char *file, int line, const char *msg, ...)
// {
//     va_list va;
//     va_start(va, msg);
//
//     if (ex) {
//         __kprintf("!!! WARN: %s !!!\nFile: %s, line: %d\n", ex, file, line);
//     } else {
//         __kprintf("!!! WARN !!!\nFile: %s, line: %d\n", file, line);
//     }
//     __vkprintf(msg, va);
//     __kprintf("\n");
//
//     va_end(va);
// }
//
// void __panic(const char *ex, const char *file, int line, const char *msg, ...)
// {
//     va_list va;
//     va_start(va, msg);
//
//     if (ex) {
//         __kprintf("!!! PANIC: %s !!!\nFile: %s, line: %d\n", ex, file, line);
//     } else {
//         __kprintf("!!! PANIC !!!\nFile: %s, line %d\n", file, line);
//     }
//     __vkprintf(msg, va);
//     __kprintf("\n");
//
//     va_end(va);
//     __stop();
// }
//
// void __assert(const char *ex, const char *file, int line, const char *msg, ...)
// {
//     va_list va;
//     va_start(va, msg);
//
//     __kprintf("!!! ASSERT FAILED: %s !!!\nFile: %s, line: %d\n", ex, file, line);
//     __vkprintf(msg, va);
//     __kprintf("\n");
//
//     va_end(va);
//     __stop();
// }
