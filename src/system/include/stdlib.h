#ifndef __SYSTEM_INCLUDE_STDLIB_H__
#define __SYSTEM_INCLUDE_STDLIB_H__


#include "common/include/inttypes.h"


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
typedef void (*salloc_callback_t)(void *entry);

typedef struct salloc_obj {
    size_t size;
    size_t align;

    salloc_callback_t ctor;
    salloc_callback_t dtor;
} salloc_obj_t;

#define SALLOC_CREATE(s, a, c, d) \
            { .size = (s), .align = (a), .ctor = (c), .dtor = (d) }

extern void salloc_create(salloc_obj_t *obj, size_t size, size_t align,
                          salloc_callback_t ctor, salloc_callback_t dtor);
extern void *salloc(struct salloc_obj *obj);
extern void sfree(void *ptr);


#endif
