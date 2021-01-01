#ifndef __LIBC_INCLUDE_FUTEX__
#define __LIBC_INCLUDE_FUTEX__


#include "common/include/inttypes.h"
#include "common/include/proc.h"


/*
 * Futex
 */
#define futex_exclusive(l) \
    for (futex_t *__m = l; __m; __m = NULL) \
        for (futex_lock(__m, 0); __m; futex_unlock(__m), __m = NULL) \
            for (int __term = 0; !__term; __term = 1)

#define futex_exclusive_spin(l, spin) \
    for (futex_t *__m = l; __m; __m = NULL) \
        for (futex_lock(__m, spin); __m; futex_unlock(__m), __m = NULL) \
            for (int __term = 0; !__term; __term = 1)

int futex_init(futex_t *f);
int futex_destory(futex_t *f);
int futex_lock(futex_t *f, int spin);
int futex_trylock(futex_t *f);
int futex_unlock(futex_t *f);


#endif
