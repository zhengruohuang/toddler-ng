#ifndef __LIBK_INCLUDE_DEBUG_H__
#define __LIBK_INCLUDE_DEBUG_H__


#include "common/include/inttypes.h"


typedef void (*cpu_stop_t)();

extern void __stop();
extern void init_libk_stop(cpu_stop_t func);

extern int kprintf(const char *fmt, ...);


/*
 * Debug
 */
#define abort()                                                 \
    do {                                                        \
        kprintf("!!! ABORT !!!\n");                             \
        kprintf("File: %s, line %d\n", __FILE__, __LINE__);     \
        __stop();                                               \
    } while (0)

#define abort_if(cond)                                          \
    do {                                                        \
        if (cond) {                                             \
            kprintf("!!! ABORT !!!\n");                         \
            kprintf("File: %s, line %d\n", __FILE__, __LINE__); \
            kprintf("Condition: %s\n", #cond);                  \
            __stop();                                           \
        }                                                       \
    } while (0)

#define warn(...)                                               \
    do {                                                        \
        kprintf("!!! WARN !!!\n");                              \
        kprintf("File: %s, line %d\n", __FILE__, __LINE__);     \
        kprintf(__VA_ARGS__);                                   \
        kprintf("\n");                                          \
        __stop();                                               \
    } while (0)

#define warn_if(cond, ...)                                      \
    do {                                                        \
        if (cond) {                                             \
            kprintf("!!! WARN !!!\n");                          \
            kprintf("Condition: %s\n", #cond);                  \
            kprintf("File: %s, line %d\n", __FILE__, __LINE__); \
            kprintf(__VA_ARGS__);                               \
            kprintf("\n");                                      \
            __stop();                                           \
        }                                                       \
    } while (0)

#define panic(...)                                              \
    do {                                                        \
        kprintf("!!! PANIC !!!\n");                             \
        kprintf("File: %s, line %d\n", __FILE__, __LINE__);     \
        kprintf(__VA_ARGS__);                                   \
        kprintf("\n");                                          \
        __stop();                                               \
    } while (0)

#define panic_if(cond, ...)                                     \
    do {                                                        \
        if (cond) {                                             \
            kprintf("!!! PANIC !!!\n");                         \
            kprintf("Condition: %s\n", #cond);                  \
            kprintf("File: %s, line %d\n", __FILE__, __LINE__); \
            kprintf(__VA_ARGS__);                               \
            kprintf("\n");                                      \
            __stop();                                           \
        }                                                       \
    } while (0)

#define assert(cond)                                            \
    do {                                                        \
        if (!(cond)) {                                          \
            kprintf("!!! ASSERTION !!!\n");                     \
            kprintf("Failed: %s\n", #cond);                     \
            kprintf("File: %s, line %d\n", __FILE__, __LINE__); \
            __stop();                                           \
        }                                                       \
    } while (0)

#define assert_if(cond, ...)                                    \
    do {                                                        \
        if (!(cond)) {                                          \
            kprintf("!!! ASSERTION !!!\n");                     \
            kprintf("Failed: %s\n", #cond);                     \
            kprintf("File: %s, line %d\n", __FILE__, __LINE__); \
            kprintf(__VA_ARGS__);                               \
            kprintf("\n");                                      \
            __stop();                                           \
        }                                                       \
    } while (0)


/*
 * Info
 */
#define info(sec, ...)                                          \
    do {                                                        \
        kprintf("[%s] ", sec);                                  \
        kprintf(__VA_ARGS__);                                   \
    } while (0)

#define info_if(cond, sec, ...)                                 \
    do {                                                        \
        if (cond) {                                             \
            kprintf("[%s] ", sec);                              \
            kprintf(__VA_ARGS__);                               \
        }                                                       \
    } while (0)


// /*
//  * Old
//  */
// #define abort()             __abort(__FILE__, __LINE__)
// #define warn(...)           __warn((void *)0, __FILE__, __LINE__, __VA_ARGS__)
// #define warn_if(ex, ...)    (void)(!(ex) || (__warn("Condition (" #ex ") met",     __FILE__, __LINE__, __VA_ARGS__), 0))
// #define warn_not(ex, ...)   (void)((ex)  || (__warn("Condition (" #ex ") not met", __FILE__, __LINE__, __VA_ARGS__), 0))
// #define panic(...)          __panic((void *)0, __FILE__, __LINE__, __VA_ARGS__)
// #define panic_if(ex, ...)   (void)(!(ex) || (__panic("Condition (" #ex ") met",     __FILE__, __LINE__, __VA_ARGS__), 0))
// #define panic_not(ex, ...)  (void)((ex)  || (__panic("Condition (" #ex ") not met", __FILE__, __LINE__, __VA_ARGS__), 0))
// #define assert(ex)          (void)((ex)  || (__assert(#ex, __FILE__, __LINE__, ""), 0))
//
// extern void __abort(const char *file, int line);
// extern void __warn(const char *ex, const char *file, int line, const char *msg, ...);
// extern void __panic(const char *ex, const char *file, int line, const char *msg, ...);
// extern void __assert(const char *ex, const char *file, int line, const char *msg, ...);


#endif
