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
    ppfn_t ppfn;
};


static int cur_dev_pfn_record_count = 0;
static struct dev_pfn_record dev_ppfn_to_vpfn[DEV_PHYS_TO_VIRT_MAPPING_SIZE];


static void record_dev_ppfn_to_vpfn(ppfn_t ppfn, ulong vpfn, int cached)
{
    panic_if(cur_dev_pfn_record_count >= DEV_PHYS_TO_VIRT_MAPPING_SIZE,
             "Run out of dev PPFN to VPFN map entries!");

    struct dev_pfn_record *record = &dev_ppfn_to_vpfn[cur_dev_pfn_record_count];
    cur_dev_pfn_record_count++;

    record->cached = cached;
    record->ppfn = ppfn;
    record->vpfn = vpfn;
}

static ulong query_dev_vpfn(ppfn_t ppfn, int cached)
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

ulong get_dev_access_window(paddr_t paddr, ulong size, int cached)
{
    ppfn_t ppfn = paddr_to_ppfn(paddr);
    panic_if(ppfn != paddr_to_ppfn(paddr + size - 1),
             "Device access window can't go across page boundary!");

    ulong vpfn = query_dev_vpfn(ppfn, 0);
    if (!vpfn) {
        int map_cached = cached == DEV_PFN_ANY_CACHED ?
                         DEV_PFN_CACHED : cached;
        map_cached = 1;

        paddr_t page_paddr = ppfn_to_paddr(ppfn);
        ulong page_vaddr = pre_valloc(1, page_paddr, map_cached);
        vpfn = vaddr_to_vpfn(page_vaddr);
        record_dev_ppfn_to_vpfn(ppfn, vpfn, map_cached);
    }

    ulong vaddr = vpfn_to_vaddr(vpfn) | get_ppage_offset(paddr);
    return vaddr;
}
