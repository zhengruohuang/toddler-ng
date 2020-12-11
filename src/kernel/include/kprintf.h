#ifndef __KERNEL_INCLUDE_KPRINTF_H__
#define __KERNEL_INCLUDE_KPRINTF_H__


#include "libk/include/kprintf.h"


extern void acquire_kprintf();
extern void release_kprintf();

extern int kprintf_unlocked(const char *fmt, ...);


#endif
