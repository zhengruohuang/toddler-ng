#ifndef __LIBC_INCLUDE_STDLIB_H__
#define __LIBC_INCLUDE_STDLIB_H__


#include "common/include/inttypes.h"


/*
 * Type conversion
 */
// atoi
#include "libk/include/string.h"


/*
 * Pseudo random
 */
// rand, rand_r, and srand
#include "libk/include/rand.h"

extern int random();
extern int random_r(unsigned int *seed);
extern void srandom(unsigned int seed);


/*
 * malloc
 */
extern void init_malloc();
extern void display_malloc();

extern void *malloc(size_t size);
extern void free(void *ptr);
extern void *calloc(size_t num, size_t size);
extern void *realloc(void *ptr, size_t new_size);


/*
 * salloc
 */
typedef int (*salloc_callback_t)(void *obj, size_t size);

typedef struct salloc_obj {
    size_t size;
    size_t align;

    salloc_callback_t ctor;
    salloc_callback_t dtor;
} salloc_obj_t;

#define SALLOC_CREATE(s, a, c, d) \
            { .size = (s), .align = (a), .ctor = (c), .dtor = (d) }

#define SALLOC_CREATE_DEFAULT(s) \
            { .size = (s), .align = 0, .ctor = salloc_zero_ctor, .dtor = NULL }

extern void salloc_create(salloc_obj_t *obj, size_t size, size_t align,
                          salloc_callback_t ctor, salloc_callback_t dtor);
extern void *salloc(struct salloc_obj *obj);
extern void sfree(void *ptr);

extern int salloc_zero_ctor(void *obj, size_t size);


/*
 * Process control
 */
extern int atexit(void (*func)(void));
extern int on_exit(void (*func)(int, void *), void *arg);

extern void _exit(int status);
extern void exit(int status);
extern void abort();


/*
 * Sort
 */


/*
 * Math
 */


#endif
