/*
 * ASID allocator
 */


#include "common/include/inttypes.h"
#include "kernel/include/struct.h"
#include "kernel/include/kernel.h"
#include "kernel/include/lib.h"


static int supports_asid = 0;
static id_obj_t asid_alloc;


/*
 * Init
 */
void init_asid()
{
    ulong limit = hal_get_asid_limit();
    if (!limit) {
        supports_asid = 0;
        return;
    }

    ulong first = 1, last = limit - 1;
    id_alloc_create(&asid_alloc, first, last);
    supports_asid = 1;

    atomic_mb();
}

int asid_supported()
{
    return supports_asid;
}


/*
 * Alloc and free
 */
ulong alloc_asid()
{
    if (!supports_asid) {
        return 0;
    }

    long asid = id_alloc(&asid_alloc);
    panic_if(asid <= 0, "Run out of ASID!\n");

    kprintf("ASID allocated: %ld\n", asid);
    return asid > 0 ? asid : 0;
}

void free_asid(ulong asid)
{
    if (!supports_asid || !asid) {
        return;
    }

    id_free(&asid_alloc, asid);
    kprintf("ASID freed: %lu\n", asid);
}
