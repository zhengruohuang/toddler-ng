#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <futex.h>
#include <sys.h>


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
        int err = obj->ctor(ptr, obj->size);
        if (err) {
            free(block);
            return NULL;
        }
    }

    return ptr;
}

void sfree(void *ptr)
{
    panic_if(!ptr, "Unable to free NULL pointer!\n");

    struct salloc_magic *block = ptr - sizeof(struct salloc_magic);
    salloc_obj_t *obj = block->obj;
    if (obj->dtor) {
        obj->dtor(ptr, obj->size);
    }

    free(block);
}

int salloc_zero_ctor(void *obj, size_t size)
{
    memzero(obj, size);
    return 0;
}
