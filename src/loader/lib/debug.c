// #include "common/include/stdarg.h"
// #include "loader/include/lprintf.h"
// #include "loader/include/lib.h"
//
//
// void __abort(const char *file, int line)
// {
//     lprintf("!!! ABORT !!!\nFile: %s, line %d\n", file, line);
//     stop();
// }
//
// void __warn(const char *ex, const char *file, int line, const char *msg, ...)
// {
//     va_list va;
//     va_start(va, msg);
//
//     if (ex) {
//         lprintf("!!! WARN: %s !!!\nFile: %s, line: %d\n", ex, file, line);
//     } else {
//         lprintf("!!! WARN !!!\nFile: %s, line: %d\n", file, line);
//     }
//     vlprintf(msg, va);
//     lprintf("\n");
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
//         lprintf("!!! PANIC: %s !!!\nFile: %s, line: %d\n", ex, file, line);
//     } else {
//         lprintf("!!! PANIC !!!\nFile: %s, line %d\n", file, line);
//     }
//     vlprintf(msg, va);
//     lprintf("\n");
//
//     va_end(va);
//     stop();
// }
//
// void __assert(const char *ex, const char *file, int line, const char *msg, ...)
// {
//     va_list va;
//     va_start(va, msg);
//
//     lprintf("!!! ASSERT FAILED: %s !!!\nFile: %s, line: %d\n", ex, file, line);
//     vlprintf(msg, va);
//     lprintf("\n");
//
//     va_end(va);
//     stop();
// }
