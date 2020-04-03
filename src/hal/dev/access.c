#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/mem.h"
#include "hal/include/dev.h"


#define DEV_PHYS_TO_VIRT_MAPPING_SIZE   32


struct dev_pfn_record {
    union {
        u8 flags;
        struct {
            u8 cached  : 1;
        };
    };

    ulong vpfn;
    ulong ppfn;
};


static int cur_dev_pfn_record_count = 0;
static struct dev_pfn_record dev_ppfn_to_vpfn[DEV_PHYS_TO_VIRT_MAPPING_SIZE];


static void record_dev_ppfn_to_vpfn(ulong ppfn, ulong vpfn, int cached)
{
    panic_if(cur_dev_pfn_record_count >= DEV_PHYS_TO_VIRT_MAPPING_SIZE,
             "Run out of dev PPFN to VPFN map entries!");

    struct dev_pfn_record *record = &dev_ppfn_to_vpfn[cur_dev_pfn_record_count];
    cur_dev_pfn_record_count++;

    record->cached = cached;
    record->ppfn = ppfn;
    record->vpfn = vpfn;
}

static ulong query_dev_vpfn(ulong ppfn, int cached)
{
    for (int i = 0; i < cur_dev_pfn_record_count; i++) {
        struct dev_pfn_record *record = &dev_ppfn_to_vpfn[i];

        if (record->ppfn == ppfn &&
            (cached == DEV_PFN_ANY_CACHED || cached == record->cached)
        ) {
            return dev_ppfn_to_vpfn[i].vpfn;
        }
    }

    return 0;
}

ulong get_dev_access_window(ulong paddr, ulong size, int cached)
{
    ulong ppfn = ADDR_TO_PFN(paddr);
    panic_if(ppfn != ADDR_TO_PFN(paddr + size - 1),
             "Device access window can't go across page boundary!");

    ulong vpfn = query_dev_vpfn(ppfn, 0);
    if (!vpfn) {
        int map_cached = cached == DEV_PFN_ANY_CACHED ?
                         DEV_PFN_CACHED : cached;
        map_cached = 1;

        ulong page_paddr = PFN_TO_ADDR(ppfn);
        ulong page_vaddr = pre_valloc(1, page_paddr, map_cached);
        vpfn = ADDR_TO_PFN(page_vaddr);
        record_dev_ppfn_to_vpfn(ppfn, vpfn, map_cached);
    }

    ulong vaddr = PFN_TO_ADDR(vpfn) | (paddr & (PAGE_SIZE - 1));
    return vaddr;
}
