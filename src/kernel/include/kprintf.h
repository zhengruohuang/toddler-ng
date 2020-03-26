#ifndef __KERNEL_INCLUDE_KPRINTF_H__
#define __KERNEL_INCLUDE_KPRINTF_H__


#include "libk/include/kprintf.h"
#include "kernel/include/atomic.h"


extern spinlock_t kprintf_lock;


#define kprintf(...)                        \
    do {                                    \
        spinlock_lock_int(&kprintf_lock);   \
        __kprintf(__VA_ARGS__);             \
        spinlock_unlock_int(&kprintf_lock); \
    } while (0)

#define vkprintf(...)                       \
    do {                                    \
        spinlock_lock_int(&kprintf_lock);   \
        __vkprintf(__VA_ARGS__);            \
        spinlock_unlock_int(&kprintf_lock); \
    } while (0)


#endif
