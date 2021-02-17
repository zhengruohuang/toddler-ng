#include "common/include/inttypes.h"
#include "common/include/page.h"
#include "hal/include/lib.h"
#include "hal/include/kernel.h"
#include "hal/include/hal.h"
#include "hal/include/mem.h"
#include "hal/include/setup.h"


static void invalidate_tlb_line(ulong asid, ulong vaddr)
{
//     kprintf("TLB shootdown @ %lx, ASID: %lx ...", vaddr, asid);
//     inv_tlb_all();
    // TODO: atomic_mb();
}

void invalidate_tlb(ulong asid, ulong vaddr, size_t size)
{
//     ulong vstart = ALIGN_DOWN(vaddr, PAGE_SIZE);
//     ulong vend = ALIGN_UP(vaddr + size, PAGE_SIZE);
//     ulong page_count = (vend - vstart) >> PAGE_BITS;
//
//     ulong i;
//     ulong vcur = vstart;
//     for (i = 0; i < page_count; i++) {
//         invalidate_tlb_line(asid, vcur);
//         vcur += PAGE_SIZE;
//     }
}

void init_tlb_mp()
{
}

void init_tlb()
{
    init_tlb_mp();
}
