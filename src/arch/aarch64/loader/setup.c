#include "common/include/inttypes.h"
#include "common/include/msr.h"
#include "common/include/mem.h"
#include "common/include/abi.h"
#include "common/include/io.h"
#include "loader/include/lib.h"
#include "loader/include/firmware.h"
#include "loader/include/boot.h"
#include "loader/include/loader.h"


/*
 * Raspi3 UART
 */
#define BCM2835_BASE            (0x3f000000ul)
#define PL011_BASE              (BCM2835_BASE + 0x201000ul)
#define PL011_DR                (PL011_BASE)
#define PL011_FR                (PL011_BASE + 0x18ul)

int arch_debug_putchar(int ch)
{
    // Wait until the UART has an empty space in the FIFO
    int ready = 0;
    while (!ready) {
        ready = !(mmio_read8(PL011_FR) & 0x20);
    }

    // Write the character to the FIFO for transmission
    mmio_write8(PL011_DR, ch & 0xff);

    return 1;
}


/*
 * Access window <--> physical addr
 */
static void *access_win_to_phys(void *vaddr)
{
    return vaddr;
}

static void *phys_to_access_win(void *paddr)
{
    return paddr;
}


/*
 * Paging
 */
static struct page_frame *root_page = NULL;

static void *alloc_page()
{
    void *paddr = memmap_alloc_phys(PAGE_SIZE, PAGE_SIZE);
    return (void *)((ulong)paddr);
}

static void *setup_page()
{
    root_page = memmap_alloc_phys(PAGE_SIZE * 2, PAGE_SIZE);
    memzero(root_page, PAGE_SIZE * 2);

    return root_page;
}

static void map_page(void *page_table, void *vaddr, void *paddr, int block,
    int cache, int exec, int write)
{
    struct page_frame *page = page_table;
    struct page_table_entry *entry = NULL;

//     for (int level = 0; level < PAGE_LEVELS; level++) {
//         int idx = (int)GET_PTE_INDEX((ulong)vaddr, level);
//         entry = &page->entries[idx];
//
//         if (level != PAGE_LEVELS - 1) {
//             if (!entry->table.valid) {
//                 page = alloc_page();
//                 memzero(page, sizeof(struct page_frame));
//
//                 entry->table.valid = 1;
//                 entry->table.table = 1;
//                 entry->table.pfn = ADDR_TO_PFN((ulong)page);
//             } else {
//                 page = (void *)PFN_TO_ADDR((ulong)entry->table.pfn);
//             }
//         }
//     }

    for (int level = 0; level <= block; level++) {
        int idx = (int)GET_PTE_INDEX((ulong)vaddr, level);
        entry = &page->entries[idx];

        if (level != block) {
            if (!entry->table.valid) {
                page = alloc_page();
                memzero(page, sizeof(struct page_frame));

                entry->table.valid = 1;
                entry->table.table = 1;
                entry->table.pfn = ADDR_TO_PFN((ulong)page);
            } else {
                page = (void *)PFN_TO_ADDR((ulong)entry->table.pfn);
            }
        }
    }

    if (block == 1 || block == 2) {
        ulong bfn = block == 1 ?
            ADDR_TO_L1BFN((ulong)paddr) : ADDR_TO_L2BFN((ulong)paddr);

        if (entry->block.valid) {
            panic_if((ulong)entry->page.pfn != bfn ||
                entry->block.attrindx != cache ? 0 : 1 ||
                entry->block.shareable != cache ? 0x3 : 0x2 ||
                entry->block.kernel_no_exec != !exec ||
                entry->block.user_no_exec != !exec ||
                entry->block.read_only != !write,
                "Trying to change an existing mapping!");
        } else {
            entry->block.valid = 1;
            entry->block.table = 0;
            entry->block.bfn = bfn;

            entry->block.user = 0;
            entry->block.accessed = 1;
            entry->block.non_global = 1;

            entry->block.attrindx = cache ? MAIR_IDX_NORMAL : MAIR_IDX_DEVICE;
            entry->block.shareable = cache ? 0x3 : 0x2;   // 0x3=Inner, 0x2=Outer
            entry->block.kernel_no_exec = entry->page.user_no_exec = !exec;
            entry->block.read_only = !write;
        }
    }

    else if (block == 3) {
        if (entry->page.valid) {
            panic_if((ulong)entry->page.pfn != ADDR_TO_PFN((ulong)paddr) ||
                entry->page.attrindx != cache ? 0 : 1 ||
                entry->page.shareable != cache ? 0x3 : 0x2 ||
                entry->page.kernel_no_exec != !exec ||
                entry->page.user_no_exec != !exec ||
                entry->page.read_only != !write,
                "Trying to change an existing mapping!");
        } else {
            entry->page.valid = 1;
            entry->page.table = 1;
            entry->page.pfn = ADDR_TO_PFN((ulong)paddr);

            entry->page.user = 0;
            entry->page.accessed = 1;
            entry->page.non_global = 1;

            entry->page.attrindx = cache ? MAIR_IDX_NORMAL : MAIR_IDX_DEVICE;
            entry->page.shareable = cache ? 0x3 : 0x2;   // 0x3=Inner, 0x2=Outer
            entry->page.kernel_no_exec = entry->page.user_no_exec = !exec;
            entry->page.read_only = !write;
        }
    }

    else {
        panic("Unsupported block level: %d\n", block);
    }
}

static int map_range(void *page_table, void *vaddr, void *paddr, ulong size,
    int cache, int exec, int write)
{
//     return 0;

    ulong vaddr_start = ALIGN_DOWN((ulong)vaddr, PAGE_SIZE);
    ulong paddr_start = ALIGN_DOWN((ulong)paddr, PAGE_SIZE);
    ulong vaddr_end = ALIGN_UP((ulong)vaddr + size, PAGE_SIZE);

    int mapped_pages = 0;
    if (vaddr_start >> 63) {
        page_table += PAGE_SIZE;
    }

    for (ulong cur_vaddr = vaddr_start, cur_paddr = paddr_start;
        cur_vaddr < vaddr_end;
    ) {
        // 1GB block
        if (ALIGNED(cur_vaddr, L1BLOCK_SIZE) &&
            ALIGNED(cur_paddr, L1BLOCK_SIZE) &&
            vaddr_end - cur_vaddr >= L1BLOCK_SIZE
        ) {
            map_page(page_table, (void *)cur_vaddr, (void *)cur_paddr, 1,
                cache, exec, write);

            mapped_pages += L1BLOCK_PAGE_COUNT;
            cur_vaddr += L1BLOCK_SIZE;
            cur_paddr += L1BLOCK_SIZE;
        }

        // 2MB block
        else if (ALIGNED(cur_vaddr, L2BLOCK_SIZE) &&
            ALIGNED(cur_paddr, L2BLOCK_SIZE) &&
            vaddr_end - cur_vaddr >= L2BLOCK_SIZE
        ) {
            map_page(page_table, (void *)cur_vaddr, (void *)cur_paddr, 2,
                cache, exec, write);

            mapped_pages += L2BLOCK_PAGE_COUNT;
            cur_vaddr += L2BLOCK_SIZE;
            cur_paddr += L2BLOCK_SIZE;
        }

        // 4KB page
        else {
            map_page(page_table, (void *)cur_vaddr, (void *)cur_paddr,
                PAGE_LEVELS - 1, cache, exec, write);

            mapped_pages++;
            cur_vaddr += PAGE_SIZE;
            cur_paddr += PAGE_SIZE;
        }
    }

    return mapped_pages;
}


/*
 * Jump to HAL
 */
static u64 get_interm_paddr_size()
{
    struct aarch64_mem_model_feature_reg mmfr;
    read_aarch64_mem_model_feature_el1(mmfr.value);

    u64 ips = mmfr.paddr_range;
    panic_if(ips & 0x8, "Too many bits for IPS!\n");

    return ips;
}

static void check_mmfr()
{
    struct aarch64_mem_model_feature_reg mmfr;
    read_aarch64_mem_model_feature_el1(mmfr.value);

    panic_if(mmfr.granule_4k != ID_AA64MMFR0_GRANULE_SUPPORT,
        "4KB page size not supported!\n");
}

static void set_mair()
{
    // Memory attribute array
    struct mem_attri_indirect_reg mair;
    mair.value = 0;

    // Normal memory, write-back
    mair.attri[MAIR_IDX_NORMAL].inner = MAIR_INNER_NORMAL;
    mair.attri[MAIR_IDX_NORMAL].outer = MAIR_OUTER_NORMAL;

    // Device, nGnRnE
    mair.attri[MAIR_IDX_DEVICE].inner = MAIR_INNER_DEV_NGNRNE;
    mair.attri[MAIR_IDX_DEVICE].outer = MAIR_OUTER_DEV;

    // Non-cacheable
    mair.attri[MAIR_IDX_NON_CACHEABLE].inner = MAIR_INNER_NON_CACHEABLE;
    mair.attri[MAIR_IDX_NON_CACHEABLE].outer = MAIR_OUTER_NON_CACHEABLE;

    kprintf("mair: %llx\n", mair.value);
    write_mem_attri_indirect_el1(mair.value);
}

static void set_tcr()
{
    struct trans_ctrl_reg tcr;
    tcr.value = 0;

    // Translation
    tcr.size_offset0 = 16;  // 48-bit vaddr
    tcr.size_offset1 = 16;

    tcr.walk_disable0 = 0;
    tcr.walk_disable1 = 1;

    tcr.ttbr_granule0 = 0x0;    // 4KB
    tcr.ttbr_granule1 = 0x2;    // 4KB

    tcr.top_byte_ignored0 = 0;  // No tagging
    tcr.top_byte_ignored1 = 0;

    tcr.interm_paddr_size = get_interm_paddr_size();

    // Caching
    tcr.inner_cacheable0 = 1;
    tcr.outer_cacheable0 = 1;
    tcr.inner_cacheable1 = 1;
    tcr.outer_cacheable1 = 1;

    tcr.shareability0 = 0x3;    // Inner
    tcr.shareability1 = 0x3;

    kprintf("tcr: %llx\n", tcr.value);
    write_trans_ctrl_el1(tcr.value);
}

static void set_ttbr(void *root_page)
{
    // Only use TTBR0
    struct trans_tab_base_reg ttbr;
    ttbr.value = 0;

    // Base addr is bits[47:1], while bit[0] is for CnP in TTBR
    ttbr.base_addr = ((ulong)root_page) >> 1;
    ttbr.cnp = 1;

    kprintf("ttbr: %llx, root @ %p\n", ttbr.value, root_page);
    write_trans_tab_base0_el1(ttbr.value);
}

static void set_sctlr()
{
    atomic_dsb();
    atomic_isb();

    struct sys_ctrl_reg sctlr;
    read_sys_ctrl_el1(sctlr.value);
//     sctlr.value = 0x30D00800ul;

    // Little endian
    sctlr.big_endian = 0;
    sctlr.big_endian_e0 = 0;

    sctlr.wxn = 0;

    // Enable all alignment check
    sctlr.sa = 0;
    sctlr.sa0 = 0;
    sctlr.align_check = 0;

    // Cache
    sctlr.data_cachable = 1;
    sctlr.instr_cacheable = 1;

    // MMU
    sctlr.mmu = 1;

    kprintf("sctlr: %llx\n", sctlr.value);
    write_sys_ctrl_el1(sctlr.value);
}

static void enable_mmu_caches(void *root_page)
{
    // FIXME: map PL011
    map_range(root_page, (void *)PL011_BASE, (void *)PL011_BASE, 0x100, 0, 0, 1);

    kprintf("Here0\n");
    check_mmfr();
    kprintf("Here1\n");
    set_mair();
    kprintf("Here2\n");
    set_tcr();
    kprintf("Here3\n");
    set_ttbr(root_page);
    kprintf("Here4\n");
    set_sctlr();
    kprintf("Here5\n");
}

typedef void (*hal_start)(struct loader_args *largs);

static void call_hal(struct loader_args *largs)
{
    ulong sp, tmp;
    struct cur_except_level_reg cur_el;
    read_cur_except_level(cur_el.value);

    kprintf("Cur EL: %d\n", (int)cur_el.except_level);
//     while (1);

    if (cur_el.except_level == 1) {
        hal_start hal = largs->hal_entry;
        hal(largs);
    }
}

static void jump_to_hal()
{
    struct loader_args *largs = get_loader_args();
    kprintf("Jump to HAL @ %p\n", largs->hal_entry);

    enable_mmu_caches(largs->page_table);
    call_hal(largs);

    while (1);
}


/*
 * Init arch
 */
static void setup_indiv_sp()
{
    struct stack_ptr_sel_reg spsel;
    read_stack_ptr_sel(spsel.value);

    spsel.indiv_sp = 1;
    write_stack_ptr_sel(spsel.value);
}

static void switch_el3_to_el2()
{
    // Set up secure config
    struct secure_config_reg scr;
    scr.value = 0;
    scr.ns = 1;
    scr.reserved2 = 3;
    scr.smd = 1;
    scr.hce = 1;
    scr.rw = 1;

    write_secure_config_el3(scr.value);

    // Set up status
    struct saved_program_status_reg spsr;
    spsr.value = 0;
    spsr.indiv_sp = 1;
    spsr.from_el = 2;
    spsr.f = spsr.i = spsr.a = spsr.d = 1;

    write_saved_program_status_el3(spsr.value);

    // Switch EL
    ulong sp, tmp;

    __asm__ __volatile__ (
        "1: mov %[sp], sp;"         // Save SP
        "   adr %[tmp], 2f;"        // Prepare ERET target
        "   msr elr_el3, %[tmp];"
        "   eret;"
        "2: mov sp, %[sp];"         // Restore SP
        : [sp] "=&r" (sp), [tmp] "=&r" (tmp)
        :
        : "memory"
    );
}

static void switch_el2_to_el1()
{
    // Enable AArch64
    struct hyp_config_reg hcr;
    hcr.value = 0;
    hcr.swio = 1;
    hcr.rw = 1;

    write_hyp_config_el2(hcr.value);

    // Set up status
    struct saved_program_status_reg spsr;
    spsr.value = 0;
    spsr.indiv_sp = 1;
    spsr.from_el = 1;
    spsr.f = spsr.i = spsr.a = spsr.d = 1;

    write_saved_program_status_el2(spsr.value);

    // Switch EL
    ulong sp = 0, tmp = 0;

    __asm__ __volatile__ (
        "1: mov %[sp], sp;"         // Save SP
        "   adr %[tmp], 2f;"        // Prepare ERET target
        "   msr elr_el2, %[tmp];"
        "   eret;"
        "2: mov sp, %[sp];"         // Restore SP
        : [sp] "=&r" (sp), [tmp] "=&r" (tmp)
        :
        : "memory"
    );
}

static void switch_to_el1()
{
    struct cur_except_level_reg cur_el;
    read_cur_except_level(cur_el.value);

    if (cur_el.except_level == 3) {
        switch_el3_to_el2();
    }

    if (cur_el.except_level >= 2) {
        switch_el2_to_el1();
    }
}

static void init_arch()
{
    setup_indiv_sp();
    switch_to_el1();

    kprintf("Arch initialized!\n");
}


/*
 * The AArch64 entry point
 */
void loader_entry(void *fdt, void *res0, void *res1, void *res2)
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

//     kprintf("fdt: %p, res0: %p, res1: %p, res2: %p\n", fdt, res0, res1, res2);
//     while (1);

    // Prepare arg
    if (is_fdt_header(fdt)) {
        //fw_args.type = FW_FDT;
        //fw_args.fdt.fdt = fdt;
        fw_args.fw_name = "fdt";
        fw_args.fw_params = fdt;
    } else {
        //fw_args.type = FW_NONE;
        fw_args.fw_name = "none";
    }

    // Prepare arch info
    funcs.reserved_stack_size = 0x8000;
    funcs.page_size = PAGE_SIZE;
    funcs.num_reserved_got_entries = ELF_GOT_NUM_RESERVED_ENTRIES;

    // Prepare funcs
    funcs.init_arch = init_arch;
    funcs.setup_page = setup_page;
    funcs.map_range = map_range;
    funcs.access_win_to_phys = access_win_to_phys;
    funcs.phys_to_access_win = phys_to_access_win;
    funcs.jump_to_hal = jump_to_hal;

    // Go to loader!
    loader(&fw_args, &funcs);
}

void loader_entry_ap()
{
}
