#include "common/include/inttypes.h"
#include "system/include/kprintf.h"
#include "system/include/thread.h"
#include "system/include/stdlib.h"
#include "libsys/include/syscall.h"
#include "libsys/include/futex.h"
#include "libk/include/string.h"


struct salloc_magic {
    struct salloc_obj *obj;
};


void *salloc(struct salloc_obj *obj)
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
    if (obj->constructor) {
        obj->constructor(ptr);
    }

    return ptr;
}

void sfree(void *ptr)
{
    if (!ptr) {
        return; // TODO: panic
    }

    struct salloc_magic *block = ptr - sizeof(struct salloc_magic);
    struct salloc_obj *obj = block->obj;
    if (obj->destructor) {
        obj->destructor(ptr);
    }

    free(block);
}
