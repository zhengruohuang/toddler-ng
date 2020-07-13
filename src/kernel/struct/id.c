#include "common/include/inttypes.h"


struct id_obj {
    ulong cur_id;
};

void id_init(struct id_obj *obj)
{
    obj->cur_id = 0;
}

ulong id_alloc(struct id_obj *obj)
{
    ulong id = obj->cur_id;
    obj->cur_id++;
    return id;
}

void id_free(struct id_obj *obj, ulong id)
{
    if (id == obj->cur_id - 1) {
        obj->cur_id--;
    }
}
