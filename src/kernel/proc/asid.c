/*
 * ASID allocator
 */


#include "common/include/inttypes.h"
#include "kernel/include/struct.h"
#include "kernel/include/lib.h"


static id_obj_t asid_alloc;


/*
 * Alloc
 */
void init_asid()
{
    ulong first = 1, last = 255;
    id_alloc_create(&asid_alloc, first, last);
}


/*
 * Alloc and free
 */
ulong alloc_asid()
{
    long asid = id_alloc(&asid_alloc);
    panic_if(asid <= 0, "Run out of ASID!\n");

    kprintf("ASID allocated: %ld\n", asid);
    return asid > 0 ? asid : 0;
}

void free_asid(ulong asid)
{
    id_free(&asid_alloc, asid);
    kprintf("ASID freed: %lu\n", asid);
}
