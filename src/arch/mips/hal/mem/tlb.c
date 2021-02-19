#include "common/include/inttypes.h"
#include "common/include/page.h"
#include "common/include/msr.h"
#include "hal/include/lib.h"
#include "hal/include/kernel.h"
#include "hal/include/hal.h"
#include "hal/include/mem.h"
#include "hal/include/int.h"
#include "hal/include/setup.h"
#include "hal/include/vecnum.h"


/*
 * TLB geometry
 */
#define KERNEL_PAGE_MASK        0xffff
#define KERNEL_PAGE_MASK_EX     0x3

#define USER_PAGE_MASK          0
#define USER_PAGE_MASK_EX       0x3

#define TLB_PFN_SHIFT_BITS      12
#define TLB_VPN2_SHIFT_BITS     13

struct tlb_config {
    int has_vtlb;
    u32 vtlb_size_mask;
    int vtlb_entry_count;

    int has_ftlb;
    u32 ftlb_ways;
    u32 ftlb_sets;
    int ftlb_entry_count;

    int tlb_entry_count;
};

static decl_per_cpu(struct tlb_config, tlb_geometry);


/*
 * TLB manipulation
 */
static int probe_tlb_index(struct tlb_config *tlb, ulong asid, ulong vpn2)
{
    int index = -1;
    struct cp0_entry_hi hi, hi_old;

    // Read the old value of HI
    read_cp0_entry_hi(hi.value);
    hi_old.value = hi.value;

    // Set ASID and vaddr
    hi.asid = asid;
    hi.vpn2 = vpn2;

    // Write HI and do a TLB probe
    write_cp0_entry_hi(hi.value);
    do_tlb_probe();
    read_cp0_index(index);

    // Restore old HI
    write_cp0_entry_hi(hi_old.value);

    return index;
}

static void write_tlb_entry(struct tlb_config *tlb, int index, int nonstd_pgsz,
                            ulong hi, ulong pm, ulong lo0, ulong lo1)
{
    u32 entry_index = index;

    // Find a random entry
    if (index < 0) {
        read_cp0_random(entry_index);

        // Limit the index to VTLB if needed
        if (nonstd_pgsz) {
            assert(tlb->has_vtlb);
            entry_index &= tlb->vtlb_size_mask;
        }
    }

    // Write to TLB
    write_cp0_entry_hi(hi);
    write_cp0_page_mask(pm);
    write_cp0_entry_lo0(lo0);
    write_cp0_entry_lo1(lo1);
    write_cp0_index(entry_index);

    // Apply the changes
    do_tlb_write();
}

static void write_invalid_vtlb_entry(struct tlb_config *tlb, int index)
{
    write_tlb_entry(tlb, index, 1, 0, 0, 0, 0);
}

static void write_invalid_ftlb_entry(struct tlb_config *tlb, int index)
{
    write_tlb_entry(tlb, index, 0, 0, 0, 0, 0);
}

static void flush_tlb(struct tlb_config *tlb)
{
    int i;

    if (tlb->has_vtlb) {
        for (i = 0; i < tlb->vtlb_entry_count; i++) {
            write_invalid_vtlb_entry(tlb, i);
        }
    }

    if (tlb->has_ftlb) {
        for (i = tlb->vtlb_entry_count; i < tlb->tlb_entry_count; i++) {
            write_invalid_ftlb_entry(tlb, i);
        }
    }
}

static void map_tlb_entry_user(struct tlb_config *tlb, ulong asid, ulong vpn2,
                               ulong ppn0, int exec0, int write0, int cache0,
                               ulong ppn1, int exec1, int write1, int cache1)
{
    struct cp0_entry_hi hi;
    struct cp0_page_mask pm;

    struct cp0_entry_lo lo0;
    struct cp0_entry_lo lo1;

    hi.value = 0;
    hi.asid = asid;
    hi.vpn2 = vpn2;

    pm.value = 0;
    pm.mask_ex = USER_PAGE_MASK_EX;
    pm.mask = USER_PAGE_MASK;

    lo0.value = 0;
    if (ppn0) {
        lo0.valid = 1;
        lo0.pfn = ppn0;
        lo0.dirty = write0;
        lo0.coherent = cache0 ? 0x3 : 0x2;
    }

    lo1.value = 0;
    if (ppn1) {
        lo1.valid = 1;
        lo1.pfn = ppn1;
        lo1.dirty = write1;
        lo1.coherent = cache1 ? 0x3 : 0x2;
    }

    int idx = probe_tlb_index(tlb, asid, vpn2);
    write_tlb_entry(tlb, idx, 0, hi.value, pm.value, lo0.value, lo1.value);
}


/*
 * CPU config
 */
static int read_cpu_config(int index, u32 *value)
{
    int success = 0;
    struct cp0_config conf;
    conf.value = 0;

    success = 1;
    read_cp0_config(0, conf.value);
    if (index == 0) goto __done;

    success = conf.has_next;
    if (!success) goto __done;
    read_cp0_config(1, conf.value);
    if (index == 1) goto __done;

    success = conf.has_next;
    if (!success) goto __done;
    read_cp0_config(2, conf.value);
    if (index == 2) goto __done;

    success = conf.has_next;
    if (!success) goto __done;
    read_cp0_config(3, conf.value);
    if (index == 3) goto __done;

    success = conf.has_next;
    if (!success) goto __done;
    read_cp0_config(4, conf.value);
    if (index == 4) goto __done;

    success = conf.has_next;
    if (!success) goto __done;
    read_cp0_config(5, conf.value);
    if (index == 5) goto __done;

    success = 0;
    goto __done;

__done:
    if (value) {
        *value = conf.value;
    }

    return success;
}


/*
 * Invalidate
 */
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

    struct tlb_config *tlb = get_per_cpu(struct tlb_config, tlb_geometry);
    flush_tlb(tlb);
}


/*
 * Refill
 */
static int int_handler_tlb_refill(struct int_context *ictxt, struct kernel_dispatch *kdi)
{
    // Get current page table and ASID
    struct page_frame *page_table = *get_per_cpu(struct page_frame *, cur_page_dir);
    ulong asid = get_cur_running_asid();

    // Get faulting vaddr
    ulong bad_addr = ictxt->error_code;
    ulong vaddr0 = vpfn_to_vaddr((vaddr_to_vpfn(bad_addr) & ~0x1ul));
    ulong vaddr1 = vpfn_to_vaddr((vaddr_to_vpfn(bad_addr) |  0x1ul));

    // Get VPN2
    ulong vpn2 = vaddr0 >> TLB_VPN2_SHIFT_BITS;

    // Get paddr
    int exec0 = 0, write0 = 0, cache0 = 0;
    int exec1 = 0, write1 = 0, cache1 = 0;
    paddr_t paddr0 = translate_attri(page_table, vaddr0, &exec0, NULL, &write0, &cache0);
    paddr_t paddr1 = translate_attri(page_table, vaddr1, &exec1, NULL, &write1, &cache1);

    // Map
    struct tlb_config *tlb = get_per_cpu(struct tlb_config, tlb_geometry);
    map_tlb_entry_user(tlb, asid, vpn2,
                       paddr_to_ppfn(paddr0), exec0, write0, cache0,
                       paddr_to_ppfn(paddr1), exec1, write1, cache1);

    //kprintf("TLB refill, vaddr @ %lx, paddr0 @ %llx, paddr1 @ %llx\n",
    //        bad_addr, (u64)paddr0, (u64)paddr1);
    return INT_HANDLE_SIMPLE;
}


/*
 * TLB init
 */
static void init_vtlb(struct tlb_config *tlb)
{
    u32 val = 0;
    struct cp0_config0 c0;
    struct cp0_config1 c1;
    struct cp0_config4 c4;

    // See if the processor has VTLB
    read_cpu_config(0, &val);
    c0.value = val;

    if (c0.mmu_type != 1 && c0.mmu_type != 4) {
        tlb->has_vtlb = 0;
        tlb->vtlb_size_mask = 0;
        kprintf("Processor does not have VTLB!\n");
        return;
    }

    tlb->has_vtlb = 1;
    tlb->vtlb_size_mask = 0x3f;

    //Find out how many VTLB entries are present
    read_cpu_config(1, &val);
    c1.value = val;
    tlb->vtlb_size_mask = c1.vtlb_size;

    // Size extension
    int success = read_cpu_config(4, &val);
    if (success) {
        c4.value = val;

        if (c0.arch_rev <= 1 && c4.mmu_ext_type != 1) {
            tlb->vtlb_size_mask |= c4.vtlb_size_ex << 6;
        } else {
            tlb->vtlb_size_mask |= c4.mmu_size_ext << 6;
        }
    }

    // Entry count
    tlb->vtlb_entry_count = tlb->vtlb_size_mask + 1;

    // Set wired limit to zero
    write_cp0_wired(0);

    kprintf("VTLB initialized, count: %d, size mask: %x\n",
            tlb->vtlb_entry_count, tlb->vtlb_size_mask);
}

static void init_ftlb(struct tlb_config *tlb)
{
    u32 val = 0;
    struct cp0_config0 c0;
    struct cp0_config4 c4;

    // See if the processor has FTLB
    assert(read_cpu_config(0, &val));
    c0.value = val;

    if (c0.mmu_type != 4) {
        tlb->has_ftlb = 0;
        tlb->ftlb_ways = tlb->ftlb_sets = 0;
        kprintf("Processor does not have FTLB!\n");
        return;
    }
    tlb->has_ftlb = 1;

    // Find out how many FTLB entries are present
    assert(read_cpu_config(4, &val));
    c4.value = val;

    tlb->ftlb_ways = c4.ftlb_ways;
    tlb->ftlb_sets = c4.ftlb_sets;

    // Set page size to 4KB
    c4.ftlb_page = 1;
    write_cp0_config(4, c4.value);

    // Verify that the processor supports our page size
    // and do not use FTLB in case the page size is not supported
    assert(read_cpu_config(4, &val));
    if (c4.value != val) {
        tlb->has_ftlb = 0;
        tlb->ftlb_ways = tlb->ftlb_sets = 0;
        kprintf("FTLB does not support 4KB page size!\n");
        return;
    }

    // Entry count
    tlb->ftlb_entry_count = tlb->ftlb_sets * tlb->ftlb_ways;

    kprintf("FTLB initialized, ways: %u, sets: %u, entry count: %x\n",
            tlb->ftlb_ways, tlb->ftlb_sets, tlb->ftlb_entry_count);
}

void init_tlb_mp()
{
    struct tlb_config *tlb = get_per_cpu(struct tlb_config, tlb_geometry);
    memzero(tlb, sizeof(struct tlb_config));

    // Init all
    init_vtlb(tlb);
    init_ftlb(tlb);

    // Total TLB entry count
    tlb->tlb_entry_count = tlb->vtlb_entry_count + tlb->ftlb_entry_count;

    // Flush TLB
    flush_tlb(tlb);

    // Set up page grain config
    struct cp0_page_grain grain;
    read_cp0_page_grain(grain.value);
    grain.value = 0;
    grain.iec = 1;
    grain.xie = 1;
    grain.rie = 1;
    write_cp0_page_grain(grain.value);

    kprintf("TLB initialized, entry count: %d\n", tlb->tlb_entry_count);
}

void init_tlb()
{
    set_int_handler(MIPS_INT_SEQ_TLB_REFILL, int_handler_tlb_refill);
    init_tlb_mp();
}
