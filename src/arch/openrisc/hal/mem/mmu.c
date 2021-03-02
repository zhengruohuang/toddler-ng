#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "common/include/page.h"
#include "common/include/msr.h"
#include "hal/include/lib.h"
#include "hal/include/kernel.h"
#include "hal/include/hal.h"
#include "hal/include/mem.h"
#include "hal/include/int.h"
#include "hal/include/setup.h"


/*
 * TLB configs
 */
static int has_hardware_walker[2] = { 0, 0 };

static int num_tlb_ways[2] = { 0, 0 };
static int num_tlb_sets[2] = { 0, 0 };
static ulong tlb_set_mask[2] = { 0, 0 };

static int protect_scheme_to_reg_idx[] = MMU_PROTECT_SCHEME_TO_REG_IDX;
static struct mmu_protect_reg_setup protect_regs[] = MMU_PROTECT_REG_SETUP;

static void setup_mmu()
{
    for (int itlb = 0; itlb <= 1; itlb++) {
        struct mmu_config_reg mmucfgr;
        read_mmucfgr(mmucfgr.value, itlb);

        int hw_walker = mmucfgr.has_hardware_walker &&
                        mmucfgr.has_ctrl_reg && mmucfgr.has_protect_reg;
        int num_ways = mmucfgr.num_tlb_ways;
        int num_sets_order = mmucfgr.num_tlb_sets_order;
        int num_sets = 1 << num_sets_order;

        if (!num_ways) {
            num_ways = 1;
        }

        has_hardware_walker[itlb] = hw_walker;
        num_tlb_ways[itlb] = num_ways;
        num_tlb_sets[itlb] = num_sets;

        ulong mask_set = num_sets - 0x1ul;
        tlb_set_mask[itlb] = mask_set;

        kprintf("%sTLB, HW walker: %d, num ways: %u, num sets: %u, mask: %lx\n",
                itlb ? "I" : "D", hw_walker, num_ways, num_sets, mask_set);

        // Set up protect reg if there's HW walker
        if (hw_walker) {
            u32 mmupr = 0;

            for (int i = 0;
                i < sizeof(protect_regs) / sizeof(struct mmu_protect_reg_setup);
                i++
            ) {
                u8 field = protect_regs[i].field;
                if (field) {
                    field--;
                    mmupr |= itlb ?
                        (u32)protect_regs[i].immupr << (field * 2) :
                        (u32)protect_regs[i].dmmupr << (field * 4);
                }
            }

            write_mmupr(mmupr, itlb);
        }
    }
}


/*
 * Page table
 */
static void *kernel_page_table = NULL;
static decl_per_cpu(void *, cur_page_table);

void *get_kernel_page_table()
{
    return kernel_page_table;
}

void set_page_table(void *page_table)
{
    void **cur_page_table_ptr = get_per_cpu(void *, cur_page_table);
    *cur_page_table_ptr = page_table;

    for (int itlb = 0; itlb <= 1; itlb++) {
        if (has_hardware_walker[itlb]) {
            struct mmu_ctrl_reg mmucr;
            mmucr.value = 0;
            mmucr.page_table_hi22 = (ulong)page_table >> 10;
            write_mmucr(mmucr.value, itlb);
        }
    }
}


/*
 * Enable and disable MMU
 */
int disable_mmu()
{
    int enabled = 0;

    struct supervision_reg sr;
    read_sr(sr.value);

    enabled = sr.dmmu_enabled || sr.immu_enabled;

    sr.dmmu_enabled = 0;
    sr.immu_enabled = 0;

    write_sr(sr.value);

    return enabled;
}

void enable_mmu()
{
    struct supervision_reg sr;
    read_sr(sr.value);

    sr.dmmu_enabled = 1;
    sr.immu_enabled = 1;

    write_sr(sr.value);
}


/*
 * TLB refill
 */
int tlb_refill(int itlb, ulong vaddr)
{
    void *page_table = *get_per_cpu(void *, cur_page_table);
    panic_if(!page_table, "Unable to obtain current page table!\n");

    int exec, read, write, cache, kernel;
    paddr_t paddr = generic_translate_attri(page_table, vaddr,
                                            &exec, &read, &write, &cache, &kernel);
    if (!paddr) {
        return -1;
    }

    int protect_type = compose_prot_type(kernel, 1, write, exec);
    int protect_field = protect_scheme_to_reg_idx[protect_type];

    //kprintf("To fill %sTLB @ %lx, PFN %lx -> %x\n",
    //        itlb ? "I" : "D", vaddr, vaddr_to_vpfn(vaddr), pte.pfn);

    int num_ways = num_tlb_ways[itlb];
    int max_lru_age = num_ways - 1;

    ulong vpfn = vaddr_to_vpfn(vaddr);
    int set = vpfn & tlb_set_mask[itlb];

    int oldest_lru_age = 0;
    int way = 0;

    for (int w = 0; w < num_ways; w++) {
        struct tlb_match_reg tlbmr;
        read_tlbmr(tlbmr.value, itlb, w, set);

        if (!tlbmr.valid) {
            way = w;
            break;
        }

        int age = tlbmr.lru_age;
        if (age > oldest_lru_age) {
            oldest_lru_age = age;
            way = w;

            if (age == max_lru_age) {
                break;
            }
        }
    }

    //kprintf("Target TLB set: %d, way: %d\n", set, way);

    struct tlb_match_reg tlbmr;
    tlbmr.value = 0;
    tlbmr.vpn = vaddr_to_vpfn(vaddr);
    tlbmr.valid = 1;
    write_tlbmr(tlbmr.value, itlb, way, set);

    ppfn_t ppfn = paddr_to_ppfn(paddr);
    struct mmu_protect_reg_setup reg = protect_regs[protect_field];

    if (itlb) {
        struct itlb_trans_reg tlbtr;
        tlbtr.value = 0;
        tlbtr.ppn = ppfn;
        tlbtr.coherent = 1;
        tlbtr.cache_disabled = !cache;
        tlbtr.weak_order = 1;
        decompose_immu_prot_type(reg.immupr,
                                 tlbtr.kernel_exec, tlbtr.user_exec);
        //tlbtr.kernel_exec = tlbtr.user_exec = 1;
        write_itlbtr(tlbtr.value, way, set);
    } else {
        struct dtlb_trans_reg tlbtr;
        tlbtr.value = 0;
        tlbtr.ppn = ppfn;
        tlbtr.coherent = 1;
        tlbtr.cache_disabled = !cache;
        tlbtr.weak_order = 1;
        decompose_dmmu_prot_type(reg.dmmupr,
                                 tlbtr.kernel_read, tlbtr.kernel_write,
                                 tlbtr.user_read, tlbtr.user_write);
        //tlbtr.kernel_read = tlbtr.kernel_write = 1;
        write_dtlbtr(tlbtr.value, way, set);
    }

    return 0;
}


/*
 * TLB flush and invalidate
 */
void flush_tlb()
{
    int ena = disable_mmu();
    atomic_mb();

    for (int itlb = 0; itlb <= 1; itlb++) {
        for (int s = 0; s < num_tlb_sets[itlb]; s++) {
            for (int w = 0; w < num_tlb_ways[itlb]; w++) {
                write_tlbmr(0, itlb, w, s);
            }
        }
    }

    atomic_ib();

    if (ena) {
        enable_mmu();
    }
}

void invalidate_tlb(ulong asid, ulong vaddr, size_t size)
{
    flush_tlb();
}


/*
 * Init
 */
void init_mmu_mp()
{
    setup_mmu();

    kernel_page_table = get_loader_args()->page_table;
    set_page_table(kernel_page_table);

    enable_mmu();
    kprintf("MMU enabled!\n");
}

void init_mmu()
{
    init_mmu_mp();
}
