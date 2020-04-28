#include "common/include/inttypes.h"
#include "common/include/page.h"
#include "common/include/abi.h"
// #include "common/include/io.h"
#include "common/include/msr.h"
#include "loader/include/lib.h"
#include "loader/include/mem.h"
#include "loader/include/boot.h"
#include "loader/include/loader.h"
#include "loader/include/firmware.h"
#include "loader/include/kprintf.h"


/*
 * Dummy UART
 */
static inline void mmio_mb()
{
//     atomic_mb();
}

static inline void mmio_write8(ulong addr, u8 val)
{
    volatile u8 *ptr = (u8 *)(addr);
    *ptr = val;
    mmio_mb();
}

#define UART_BASE_ADDR          (0x90000000)
#define UART_DATA_ADDR          (UART_BASE_ADDR + 0x0)

static int or1k_putchar(int ch)
{
    mmio_write8(UART_DATA_ADDR, (u8)ch & 0xff);
    return 1;
}


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
 * Paging
 */
static struct page_frame *root_page = NULL;

static void *alloc_page()
{
    paddr_t paddr = memmap_alloc_phys(PAGE_SIZE, PAGE_SIZE);
    return cast_paddr_to_ptr(paddr);
}

static void *setup_page()
{
    root_page = alloc_page();
    memzero(root_page, PAGE_SIZE);

    return root_page;
}

static void map_page(void *page_table, ulong vaddr, paddr_t paddr, int block,
    int cache, int exec, int write)
{
    int protect_type = COMPOSE_PROTECT_TYPE(1, 1, write, exec);
    int protect_field = protect_scheme_to_reg_idx[protect_type];
    panic_if(!protect_field, "Unsupported protection type!\n");

    struct l1table *l1table = page_table;
    int l1idx = GET_L1PTE_INDEX((ulong)vaddr);
    struct page_table_entry *l1entry = &l1table->entries[l1idx];
    int l1entry_modified = 0;

    // 1-level block mapping
    if (block) {
        if (l1entry->protect_field) {
            panic_if((ulong)l1entry->pfn != (u32)paddr_to_ppfn(paddr) ||
                l1entry->protect_field != protect_field ||
                l1entry->cache_disabled != !cache,
                "Trying to change an existing mapping from PFN %x to %x!",
                l1entry->pfn, (u32)paddr_to_ppfn(paddr));
        }

        else {
            l1entry->value = 0;
            l1entry->pfn = (u32)paddr_to_ppfn(paddr);
            l1entry->protect_field = protect_field;
            l1entry->coherent = 1;
            l1entry->cache_disabled = !cache;
            l1entry->write_back = 1;
            l1entry->weak_order = 1;
        }

        l1entry_modified = 1;
    }

    // 2-level page mapping
    else {
        struct l2table *l2table = NULL;
        if (!l1entry->protect_field) {
            l2table = alloc_page();
            memzero(l2table, L2PAGE_TABLE_SIZE);

            l1entry->has_next = 1;
            l1entry->pfn = (u32)ptr_to_vpfn(l2table);
            l1entry->protect_field =
                protect_scheme_to_reg_idx[COMPOSE_PROTECT_TYPE(1, 1, 1, 1)];


            l1entry_modified = 1;
        } else {
            assert(l1entry->has_next);
            l2table = vpfn_to_ptr((ulong)l1entry->pfn);
        }

        int l2idx = GET_L2PTE_INDEX((ulong)vaddr);
        struct page_table_entry *l2entry = &l2table->entries[l2idx];

        if (l2entry->protect_field) {
            panic_if((ulong)l2entry->pfn != (u32)paddr_to_ppfn(paddr),
                     "Trying to change an existing mapping from PFN %x to %x!",
                     l2entry->pfn, (u32)paddr_to_ppfn(paddr));

            panic_if((int)l2entry->cache_disabled != !cache ||
                     l2entry->protect_field != protect_field,
                     "Trying to change cacheable attribute!");
        }

        else {
            l2entry->value = 0;
            l2entry->pfn = (u32)paddr_to_ppfn(paddr);
            l2entry->protect_field = protect_field;
            l2entry->coherent = 1;
            l2entry->cache_disabled = !cache;
            l2entry->write_back = 1;
            l2entry->weak_order = 1;
        }
    }

    if (l1entry_modified) {
        for (int i = 0; i < L1PAGE_TABLE_SIZE_IN_PAGES - 1; i++) {
            l1table->entries_dup[i][l1idx].value = l1entry->value;
        }
    }
}

static int map_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size,
        int cache, int exec, int write)
{
    kprintf("Map, page_dir_pfn: %p, vaddr @ %p, paddr @ %llx, size: %ld, cache: %d, exec: %d, write: %d\n",
       page_table, vaddr, (u64)paddr, size, cache, exec, write
    );

    ulong vaddr_start = align_down_vaddr(vaddr, PAGE_SIZE);
    paddr_t paddr_start = align_down_paddr(paddr, PAGE_SIZE);
    ulong vaddr_end = align_up_vaddr(vaddr + size, PAGE_SIZE);

    int mapped_pages = 0;

    paddr_t cur_paddr = paddr_start;
    for (ulong cur_vaddr = vaddr_start; cur_vaddr < vaddr_end; ) {
        // 1MB block
        if (is_vaddr_aligned(cur_vaddr, L1BLOCK_SIZE) &&
            is_paddr_aligned(cur_paddr, L1BLOCK_SIZE) &&
            vaddr_end - cur_vaddr >= L1BLOCK_SIZE
        ) {
            //kprintf("1MB, vaddr @ %lx, paddr @ %lx, len: %d\n", cur_vaddr, cur_paddr, L1BLOCK_SIZE);

            map_page(page_table, cur_vaddr, cur_paddr, 1, cache, exec, write);
            mapped_pages += L1BLOCK_PAGE_COUNT;
            cur_vaddr += L1BLOCK_SIZE;
            cur_paddr += L1BLOCK_SIZE;
        }

        // 4KB page
        else {
            map_page(page_table, cur_vaddr, cur_paddr, 0, cache, exec, write);
            mapped_pages++;
            cur_vaddr += PAGE_SIZE;
            cur_paddr += PAGE_SIZE;
        }
    }

    return mapped_pages;
}

static u32 translate_root_page_table(ulong vaddr)
{
    u32 translated_pte = 0;

    struct l1table *l1table = (void *)root_page;
    int l1idx = GET_L1PTE_INDEX((ulong)vaddr);

    struct page_table_entry *l1entry = &l1table->entries[l1idx];
    panic_if(!l1entry->protect_field,
             "Vaddr not mapped in L1 table: %lx\n", vaddr);

    // 1-level block mapping
    if (!l1entry->has_next) {
        struct page_table_entry pte;
        pte.value = l1entry->value;
        pte.pfn += GET_L2PTE_INDEX(vaddr);
        translated_pte = pte.value;
    }

    // 2-level page mapping
    else {
        struct l2table *l2table = vpfn_to_ptr((ulong)l1entry->pfn);
        int l2idx = GET_L2PTE_INDEX((ulong)vaddr);

        struct page_table_entry *l2entry = &l2table->entries[l2idx];
        panic_if(!l2entry->protect_field,
                 "Vaddr not mapped in L2 table: %lx\n", vaddr);

        translated_pte = l2entry->value;
    }

    return translated_pte;
}


/*
 * TLB refill
 */
static void tlb_refill(ulong vaddr, int itlb)
{
    struct page_table_entry pte;
    pte.value = translate_root_page_table(vaddr);

    //kprintf("To fill %sTLB @ %lx, PFN %lx -> %x\n",
    //        itlb ? "I" : "D", vaddr, vaddr_to_vpfn(vaddr), pte.pfn);
    //while (1);

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

    struct mmu_protect_reg_setup reg = protect_regs[pte.protect_field];

    if (itlb) {
        struct itlb_trans_reg tlbtr;
        tlbtr.value = 0;
        tlbtr.ppn = pte.pfn;
        tlbtr.coherent = pte.coherent;
        tlbtr.cache_disabled = pte.cache_disabled;
        tlbtr.weak_order = pte.weak_order;
        DECOMPOSE_IMMU_PROTECT_TYPE(reg.immupr,
                                    tlbtr.kernel_exec, tlbtr.user_exec);
        //tlbtr.kernel_exec = tlbtr.user_exec = 1;
        write_itlbtr(tlbtr.value, way, set);
    } else {
        struct dtlb_trans_reg tlbtr;
        tlbtr.value = 0;
        tlbtr.ppn = pte.pfn;
        tlbtr.coherent = pte.coherent;
        tlbtr.cache_disabled = pte.cache_disabled;
        tlbtr.weak_order = pte.weak_order;
        DECOMPOSE_DMMU_PROTECT_TYPE(reg.dmmupr,
                                    tlbtr.kernel_read, tlbtr.kernel_write,
                                    tlbtr.user_read, tlbtr.user_write);
        //tlbtr.kernel_read = tlbtr.kernel_write = 1;
        write_dtlbtr(tlbtr.value, way, set);
    }
}


/*
 * Exception Handler
 */
static char *except_names[] = {
    "None",
    "Reset",
    "Bus Error",
    "Data Page Fault",
    "Instr Page Fault",
    "Tick Timer",
    "Alignment",
    "Illegal Instr",
    "External Interrupt",
    "DTLB Miss",
    "ITLB Miss",
    "Range",
    "Syscall",
    "Floating Point",
    "Trap",
    "Reserved"
};

void loader_except(ulong code)
{
    ulong epc = 0;
    read_epcr0(epc);

    ulong eear = 0;
    read_eear0(eear);

    char *ename = "Unknown";
    if (code < sizeof(except_names) / sizeof(char *)) {
        ename = except_names[code];
    }
    kprintf("Except #%d: %s, EPC @ %lx, EEAR @ %lx\n", code, ename, epc, eear);

    switch (code) {
    case 9:
        tlb_refill(eear, 0);
        break;
    case 10:
        tlb_refill(eear, 1);
        break;
    default:
        panic("Unsupported except #%d: %s, EPC @ %lx, EEAR @ %lx\n",
              code, ename, epc, eear);
        break;
    }

    //kprintf("Handled!\n");
//     while (1);
}


/*
 * Jump to HAL
 */
typedef void (*hal_start_t)(struct loader_args *largs, int mp);

static void call_hal(struct loader_args *largs, int mp)
{
    hal_start_t hal = largs->hal_entry;
    hal(largs, 0);
}

static void jump_to_hal()
{
    struct loader_args *largs = get_loader_args();
    kprintf("Jump to HAL @ %p\n", largs->hal_entry);

    call_hal(largs, 0);
}


/*
 * Finalize
 */
static void final_memmap()
{
    // FIXME: simply assume all the memory is direct mapped
    // may not be true on some machines
    u64 start = 0;
    u64 size = get_memmap_range(&start);
    tag_memmap_region(start, size, MEMMAP_TAG_DIRECT_MAPPED);
}

static void enable_mmu_and_caches()
{
    for (int itlb = 0; itlb <= 1; itlb++) {
        if (has_hardware_walker[itlb]) {
            struct mmu_ctrl_reg mmucr;
            mmucr.value = 0;
            mmucr.page_table_hi22 = (ulong)root_page >> 10;
            write_mmucr(mmucr.value, itlb);
        }
    }

    struct supervision_reg sr;
    read_sr(sr.value);

    sr.dmmu_enabled = 1;
    sr.immu_enabled = 1;
    sr.dcache_enabled = 1;
    sr.icache_enabled = 1;

    write_sr(sr.value);
    kprintf("MMU enabled!\n");
    //while (1);
}

static void final_arch()
{
    // FIXME: map UART
    map_range(root_page, UART_BASE_ADDR, UART_BASE_ADDR, 0x100, 0, 0, 1);

    // Enable MMU and caches
    enable_mmu_and_caches();
}


/*
 * Init
 */
static void check_capability()
{
    // Toddler requires the CPU to have at least one shadow register file
    // for exception handling
    struct cpu_config_reg cpucfgr;
    read_cpucfgr(cpucfgr.value);
    panic_if(!cpucfgr.num_shadow_gpr_files,
             "Toddler requires at least one shadow register file!\n");
}

static void init_arch()
{
    check_capability();
    setup_mmu();
}

static void init_libk()
{
    init_libk_putchar(or1k_putchar);
}


/*
 * The dummy entry point
 */
void loader_entry()
{
    /*
     * BSS will be initialized at the beginning of loader() func
     * Thus no global vars are safe to access before init_arch() gets called
     * In other words, do not use global vars in loader_entry()
     */

    struct loader_arch_funcs funcs;
    struct firmware_args fw_args;

    // Safe to call lib funcs as they don't use any global vars
    memzero(&funcs, sizeof(struct loader_arch_funcs));
    memzero(&fw_args, sizeof(struct firmware_args));

    // Prepare arg
    fw_args.fw_name = "none";

    // Prepare funcs
    funcs.init_libk = init_libk;
    funcs.init_arch = init_arch;
    funcs.setup_page = setup_page;
    funcs.map_range = map_range;
    funcs.final_memmap = final_memmap;
    funcs.final_arch = final_arch;
    funcs.jump_to_hal = jump_to_hal;

    // Go to loader!
    loader(&fw_args, &funcs);
}

void loader_entry_mp()
{
}
