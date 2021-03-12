#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "common/include/page.h"
#include "common/include/msr.h"
#include "hal/include/hal.h"
#include "hal/include/mem.h"


/*
 * TLB
 */
static inline void fence_vma(ulong asid, ulong vaddr)
{
    __asm__ __volatile__ (
        "sfence.vma %[vaddr], %[asid];"
        :
        : [asid] "r" (asid), [vaddr] "r" (vaddr)
        : "memory"
    );
}

static inline void fence_vma_all()
{
    __asm__ __volatile__ (
        "sfence.vma x0, x0;"
        :
        :
        : "memory"
    );
}

void invalidate_tlb(ulong asid, ulong vaddr, size_t size)
{
    atomic_mb();
    fence_vma(asid, vaddr);
    atomic_ib();
}

void flush_tlb()
{
    atomic_mb();
    fence_vma_all();
    atomic_ib();
}


/*
 * ASID
 */
static ulong max_asid = 0;

static inline ulong find_max_asid()
{
#ifdef MODEL_GENERIC
    // TODO
    return 0;
#else
    return 0;
#endif
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
    struct supervisor_addr_trans_prot_reg stap;
    stap.value = 0;

    stap.pfn = paddr_to_ppfn(cast_ptr_to_paddr(page_table));
    stap.asid = asid;
#if (defined(ARCH_RISCV64) && MAX_NUM_VADDR_BITS == 48)
    stap.mode = STAP_MODE_SV48;
#elif (defined(ARCH_RISCV64) && MAX_NUM_VADDR_BITS == 39)
    stap.mode = STAP_MODE_SV39;
#elif (defined(ARCH_RISCV32) && MAX_NUM_VADDR_BITS == 32)
    stap.mode = STAP_MODE_SV32;
#else
    #error "Unsupported arch or max vaddr width!"
#endif

    atomic_mb();
    write_satp(stap.value);
    fence_vma_all();
    atomic_ib();
}


/*
 * Init
 */
void init_mmu()
{
    // ASID
    max_asid = find_max_asid();

    struct hal_arch_funcs *funcs = get_hal_arch_funcs();
    funcs->asid_limit = max_asid;

    // Page table
    kernel_page_table = get_loader_args()->page_table;
}

void init_mmu_mp()
{
}
