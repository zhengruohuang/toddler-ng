#include "common/include/inttypes.h"
#include "hal/include/lib.h"
#include "hal/include/mem.h"
#include "hal/include/hal.h"


/*
 * TKB
 */
static void invalidate_tlb_page(ulong asid, ulong vaddr)
{
    __asm__ __volatile__ (
#if (ARCH_WIDTH == 64)
        "invlpg (%%rax);"
#elif (ARCH_WIDTH == 32)
        "invlpg (%%eax);"
#endif
        :
        : "a" (vaddr)
        : "memory", "cc"
    );
}

void invalidate_tlb(ulong asid, ulong vaddr, size_t size)
{
    ulong vstart = align_down_vaddr(vaddr, PAGE_SIZE);
    ulong vend = align_up_vaddr(vaddr + size, PAGE_SIZE);

    for (ulong vcur = vstart; vcur < vend; vcur += PAGE_SIZE) {
        invalidate_tlb_page(asid, vcur);
    }
}

void flush_tlb()
{
    __unused_var ulong ax = 0;

    __asm__ __volatile__ (
#if (ARCH_WIDTH == 64)
        "movq %%cr3, %%rax;"
        "movq %%rax, %%cr3;"
#elif (ARCH_WIDTH == 32)
        "movl %%cr3, %%eax;"
        "movl %%eax, %%cr3;"
#endif
        "jmp    1f;"
        "1: nop;"
        : "=a" (ax)
        :
        : "memory", "cc"
    );
}


/*
 * Page table
 */
static void *kernel_page_table = NULL;

void *get_kernel_page_table()
{
    return kernel_page_table;
}

void switch_page_table(void *page_table, ulong asid)
{
    //kprintf("switch page table @ %p\n", page_table);

    __asm__ __volatile__ (
#if (ARCH_WIDTH == 64)
        "movq   %%rax, %%cr3;"
#elif (ARCH_WIDTH == 32)
        "movl   %%eax, %%cr3;"
#endif
        "jmp    1f;"
        "1: nop;"
        :
        : "a" ((ulong)page_table)
        : "memory", "cc"
    );
}


/*
 * ASID
 */
static inline ulong find_max_asid()
{
    return 0;
}


/*
 * Init
 */
void init_mmu_mp()
{
    // Make sure ASID is supported on this processor
}

void init_mmu()
{
    // ASID
    ulong max_asid = find_max_asid();

    struct hal_arch_funcs *funcs = get_hal_arch_funcs();
    funcs->asid_limit = max_asid;

    // Page table
    kernel_page_table = get_loader_args()->page_table;
}
