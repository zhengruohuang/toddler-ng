#ifndef __LIBK_INCLUDE_DEBUG_H__
#define __LIBK_INCLUDE_DEBUG_H__


#include "common/include/inttypes.h"


typedef void (*cpu_stop_t)();


/*
 * Debug
 */
#define abort()             __abort(__FILE__, __LINE__)
#define warn(...)           __warn((void *)0, __FILE__, __LINE__, __VA_ARGS__)
#define warn_if(ex, ...)    (void)(!(ex) || (__warn("Condition (" #ex ") met",     __FILE__, __LINE__, __VA_ARGS__), 0))
#define warn_not(ex, ...)   (void)((ex)  || (__warn("Condition (" #ex ") not met", __FILE__, __LINE__, __VA_ARGS__), 0))
#define panic(...)          __panic((void *)0, __FILE__, __LINE__, __VA_ARGS__)
#define panic_if(ex, ...)   (void)(!(ex) || (__panic("Condition (" #ex ") met",     __FILE__, __LINE__, __VA_ARGS__), 0))
#define panic_not(ex, ...)  (void)((ex)  || (__panic("Condition (" #ex ") not met", __FILE__, __LINE__, __VA_ARGS__), 0))
#define assert(ex)          (void)((ex)  || (__assert(#ex, __FILE__, __LINE__, ""), 0))

extern void init_libk_stop(cpu_stop_t func);

extern void __abort(const char *file, int line);
extern void __warn(const char *ex, const char *file, int line, const char *msg, ...);
extern void __panic(const char *ex, const char *file, int line, const char *msg, ...);
extern void __assert(const char *ex, const char *file, int line, const char *msg, ...);


#endif
