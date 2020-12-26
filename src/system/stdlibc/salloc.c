#include "common/include/inttypes.h"
#include "system/include/kprintf.h"
#include "system/include/thread.h"
#include "system/include/stdlib.h"
#include "libsys/include/syscall.h"
#include "libsys/include/futex.h"
#include "libk/include/string.h"


struct salloc_magic {
    salloc_obj_t *obj;
};


void salloc_create(salloc_obj_t *obj, size_t size, size_t align, salloc_callback_t ctor, salloc_callback_t dtor)
{
    if (!obj) {
        return;
    }

    obj->size = size;
    obj->align = align;
    obj->ctor = ctor;
    obj->dtor = dtor;
}

void *salloc(salloc_obj_t *obj)
{
    if (!obj) {
        return NULL;
    }

    size_t real_size = obj->size + sizeof(struct salloc_magic);
    struct salloc_magic *block = malloc(real_size);
    if (!block) {
        return NULL;
    }

    block->obj = obj;
    void *ptr = (void *)block + sizeof(struct salloc_magic);
    if (obj->ctor) {
        obj->ctor(ptr);
    }

    return ptr;
}

void sfree(void *ptr)
{
    if (!ptr) {
        return; // TODO: panic
    }

    struct salloc_magic *block = ptr - sizeof(struct salloc_magic);
    salloc_obj_t *obj = block->obj;
    if (obj->dtor) {
        obj->dtor(ptr);
    }

    free(block);
}
