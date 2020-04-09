#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "hal/include/kprintf.h"
#include "hal/include/mem.h"
#include "hal/include/lib.h"


#define MEMPOOL_CHUNK_PAGE_COUNT 1

static void *mempool = NULL;

static ulong mempool_start = 0;
static ulong mempool_limit = 0;


void mempool_free(void *ptr)
{
    warn("Mempool does not support free, request ignored @ %p\n", ptr);
}

void *mempool_alloc(size_t size)
{
    if (!mempool || mempool_start + size > mempool_limit) {
        ppfn_t ppfn = pre_palloc(MEMPOOL_CHUNK_PAGE_COUNT);
        paddr_t paddr = ppfn_to_paddr(ppfn);
        ulong vaddr = pre_valloc(MEMPOOL_CHUNK_PAGE_COUNT, paddr, 1);

        mempool = (void *)vaddr;
        mempool_start = 0;
        mempool_limit = MEMPOOL_CHUNK_PAGE_COUNT * PAGE_SIZE;

        kprintf("Pool created @ %p\n", mempool);
    }

    panic_if(!mempool, "Unable to allocate pool space");
    panic_if(mempool_start + size > mempool_limit, "Object too big: %ld\n", size);

    void *result = (void *)mempool + mempool_start;
    mempool_start += ALIGN_UP(size, sizeof(ulong));

    return result;
}
