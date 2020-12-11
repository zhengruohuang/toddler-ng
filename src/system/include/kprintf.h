#ifndef __SYSTEM_INCLUDE_KPRINTF_H__
#define __SYSTEM_INCLUDE_KPRINTF_H__


#include "libk/include/kprintf.h"


extern void init_kprintf();
extern int kprintf_unlocked(const char *fmt, ...);


#endif
