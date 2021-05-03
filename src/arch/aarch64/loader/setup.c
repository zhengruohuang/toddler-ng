#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "common/include/atomic.h"
#include "common/include/msr.h"
#include "common/include/page.h"
#include "loader/include/lib.h"
#include "loader/include/mem.h"
#include "loader/include/boot.h"
#include "loader/include/loader.h"
#include "loader/include/kprintf.h"
#include "loader/include/firmware.h"


/*
 * Raspi3 UART
 */
#define BCM2835_BASE            (0x3f000000ul)
#define PL011_BASE              (BCM2835_BASE + 0x201000ul)
#define PL011_DR                (PL011_BASE)
#define PL011_FR                (PL011_BASE + 0x18ul)

static inline u8 _raw_mmio_read8(ulong addr)
{
    u8 val = 0;
    volatile u8 *ptr = (u8 *)addr;
    atomic_mb();
    val = *ptr;
    atomic_mb();
    return val;
}

static inline void _raw_mmio_write8(ulong addr, u8 val)
{
    volatile u8 *ptr = (u8 *)addr;
    atomic_mb();
    *ptr = val;
    atomic_mb();
}

static int raspi3_putchar(int ch)
{
    // Wait until the UART has an empty space in the FIFO
    int ready = 0;
    while (!ready) {
        ready = !(_raw_mmio_read8(PL011_FR) & 0x20);
    }

    // Write the character to the FIFO for transmission
    _raw_mmio_write8(PL011_DR, ch & 0xff);

    return 1;
}


/*
 * Paging
 */
static struct page_frame *root_page = NULL;

static void *setup_page()
{
    paddr_t paddr = memmap_alloc_phys(PAGE_SIZE * 2, PAGE_SIZE);
    root_page = cast_paddr_to_ptr(paddr);
    memzero(root_page, PAGE_SIZE * 2);

    return root_page;
}

static int map_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size,
                     int cache, int exec, int write)
{
    kprintf("Map range @ %lx -> %llx, size: %lx\n", vaddr, (u64)paddr, size);

    if (vaddr >> 63) {
        page_table += PAGE_SIZE;
    }
    return generic_map_range(page_table, vaddr, paddr, size, cache, exec, write, 1, 0);
}


// static void map_page(void *page_table, void *vaddr, void *paddr, int block,
//     int cache, int exec, int write)
// {
//     struct page_frame *page = page_table;
//     struct page_table_entry *entry = NULL;
//
// //     for (int level = 0; level < PAGE_LEVELS; level++) {
// //         int idx = (int)GET_PTE_INDEX((ulong)vaddr, level);
// //         entry = &page->entries[idx];
// //
// //         if (level != PAGE_LEVELS - 1) {
// //             if (!entry->table.valid) {
// //                 page = alloc_page();
// //                 memzero(page, sizeof(struct page_frame));
// //
// //                 entry->table.valid = 1;
// //                 entry->table.table = 1;
// //                 entry->table.pfn = ADDR_TO_PFN((ulong)page);
// //             } else {
// //                 page = (void *)PFN_TO_ADDR((ulong)entry->table.pfn);
// //             }
// //         }
// //     }
//
//     for (int level = 0; level <= block; level++) {
//         int idx = (int)GET_PTE_INDEX((ulong)vaddr, level);
//         entry = &page->entries[idx];
//
//         if (level != block) {
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
//
//     if (block == 1 || block == 2) {
//         ulong bfn = block == 1 ?
//             ADDR_TO_L1BFN((ulong)paddr) : ADDR_TO_L2BFN((ulong)paddr);
//
//         if (entry->block.valid) {
//             panic_if((ulong)entry->page.pfn != bfn ||
//                 entry->block.attrindx != cache ? 0 : 1 ||
//                 entry->block.shareable != cache ? 0x3 : 0x2 ||
//                 entry->block.kernel_no_exec != !exec ||
//                 entry->block.user_no_exec != !exec ||
//                 entry->block.read_only != !write,
//                 "Trying to change an existing mapping!");
//         } else {
//             entry->block.valid = 1;
//             entry->block.table = 0;
//             entry->block.bfn = bfn;
//
//             entry->block.user = 0;
//             entry->block.accessed = 1;
//             entry->block.non_global = 1;
//
//             entry->block.attrindx = cache ? MAIR_IDX_NORMAL : MAIR_IDX_DEVICE;
//             entry->block.shareable = cache ? 0x3 : 0x2;   // 0x3=Inner, 0x2=Outer
//             entry->block.kernel_no_exec = entry->page.user_no_exec = !exec;
//             entry->block.read_only = !write;
//         }
//     }
//
//     else if (block == 3) {
//         if (entry->page.valid) {
//             panic_if((ulong)entry->page.pfn != ADDR_TO_PFN((ulong)paddr) ||
//                 entry->page.attrindx != cache ? 0 : 1 ||
//                 entry->page.shareable != cache ? 0x3 : 0x2 ||
//                 entry->page.kernel_no_exec != !exec ||
//                 entry->page.user_no_exec != !exec ||
//                 entry->page.read_only != !write,
//                 "Trying to change an existing mapping!");
//         } else {
//             entry->page.valid = 1;
//             entry->page.table = 1;
//             entry->page.pfn = ADDR_TO_PFN((ulong)paddr);
//
//             entry->page.user = 0;
//             entry->page.accessed = 1;
//             entry->page.non_global = 1;
//
//             entry->page.attrindx = cache ? MAIR_IDX_NORMAL : MAIR_IDX_DEVICE;
//             entry->page.shareable = cache ? 0x3 : 0x2;   // 0x3=Inner, 0x2=Outer
//             entry->page.kernel_no_exec = entry->page.user_no_exec = !exec;
//             entry->page.read_only = !write;
//         }
//     }
//
//     else {
//         panic("Unsupported block level: %d\n", block);
//     }
// }
//
// static int map_range(void *page_table, void *vaddr, void *paddr, ulong size,
//     int cache, int exec, int write)
// {
// //     return 0;
//
//     ulong vaddr_start = ALIGN_DOWN((ulong)vaddr, PAGE_SIZE);
//     ulong paddr_start = ALIGN_DOWN((ulong)paddr, PAGE_SIZE);
//     ulong vaddr_end = ALIGN_UP((ulong)vaddr + size, PAGE_SIZE);
//
//     int mapped_pages = 0;
//     if (vaddr_start >> 63) {
//         page_table += PAGE_SIZE;
//     }
//
//     for (ulong cur_vaddr = vaddr_start, cur_paddr = paddr_start;
//         cur_vaddr < vaddr_end;
//     ) {
//         // 1GB block
//         if (ALIGNED(cur_vaddr, L1BLOCK_SIZE) &&
//             ALIGNED(cur_paddr, L1BLOCK_SIZE) &&
//             vaddr_end - cur_vaddr >= L1BLOCK_SIZE
//         ) {
//             map_page(page_table, (void *)cur_vaddr, (void *)cur_paddr, 1,
//                 cache, exec, write);
//
//             mapped_pages += L1BLOCK_PAGE_COUNT;
//             cur_vaddr += L1BLOCK_SIZE;
//             cur_paddr += L1BLOCK_SIZE;
//         }
//
//         // 2MB block
//         else if (ALIGNED(cur_vaddr, L2BLOCK_SIZE) &&
//             ALIGNED(cur_paddr, L2BLOCK_SIZE) &&
//             vaddr_end - cur_vaddr >= L2BLOCK_SIZE
//         ) {
//             map_page(page_table, (void *)cur_vaddr, (void *)cur_paddr, 2,
//                 cache, exec, write);
//
//             mapped_pages += L2BLOCK_PAGE_COUNT;
//             cur_vaddr += L2BLOCK_SIZE;
//             cur_paddr += L2BLOCK_SIZE;
//         }
//
//         // 4KB page
//         else {
//             map_page(page_table, (void *)cur_vaddr, (void *)cur_paddr,
//                 PAGE_LEVELS - 1, cache, exec, write);
//
//             mapped_pages++;
//             cur_vaddr += PAGE_SIZE;
//             cur_paddr += PAGE_SIZE;
//         }
//     }
//
//     return mapped_pages;
// }


/*
 * Jump to HAL
 */
static u64 get_interm_paddr_size()
{
    struct aarch64_mem_model_feature_reg mmfr;
    read_aarch64_mem_model_feature_el1(mmfr.value);

    u64 ips = mmfr.paddr_range;
    kprintf("Interm paddr size: %llx\n", ips);
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

    kprintf("Mem attri array MAIR: %llx\n", mair.value);
    write_mem_attri_indirect_el1(mair.value);
}

static void set_tcr()
{
    struct trans_ctrl_reg tcr;
    tcr.value = 0;

    // Translation
    tcr.size_offset0 = 16;  // 39-bit // 48-bit vaddr
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

    kprintf("Trans ctrl TCR: %llx\n", tcr.value);
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

    kprintf("Trans table base TBBR: %llx, page table @ %p\n",
            ttbr.value, root_page);
    write_trans_tab_base0_el1(ttbr.value);
}

static void set_sctlr()
{
    atomic_dsb();
    atomic_isb();

    struct sys_ctrl_reg sctlr;
    read_sys_ctrl_el1(sctlr.value);

    // Little endian
    sctlr.big_endian = 0;
    sctlr.big_endian_e0 = 0;

    // Write doesn't imply no execute
    sctlr.wxn = 0;

    // Enable all alignment check
    sctlr.sa = 1;
    sctlr.sa0 = 1;
    sctlr.align_check = 1;

    // Enable caches
    sctlr.data_cachable = 1;
    sctlr.instr_cacheable = 1;

    // Enable MMU
    sctlr.mmu = 1;

    kprintf("Sys ctrl SCTLR: %llx\n", sctlr.value);
    write_sys_ctrl_el1(sctlr.value);
}

static void enable_mmu_caches(void *root_page)
{
    check_mmfr();
    set_mair();
    set_tcr();
    set_ttbr(root_page);
    set_sctlr();
}

typedef void (*hal_start)(struct loader_args *largs, int mp);

static void call_hal(struct loader_args *largs, int mp)
{
    struct cur_except_level_reg cur_el;
    read_cur_except_level(cur_el.value);

    kprintf("Cur EL: %d\n", (int)cur_el.except_level);
    panic_if(cur_el.except_level != 1, "Current EL must be 1!\n");

    hal_start hal = largs->hal_entry;
    hal(largs, mp);
}

static void jump_to_hal()
{
    struct loader_args *largs = get_loader_args();
    kprintf("Jump to HAL @ %p\n", largs->hal_entry);

    enable_mmu_caches(largs->page_table);
    call_hal(largs, 0);
}

static void jump_to_hal_mp()
{
}


/*
 * Finalize
 */
static void final_memmap()
{
    // Direct map as much memory as possible
    u64 start = 0;
    u64 size = get_memmap_range(&start);
    if (start + size > USER_VADDR_LIMIT) {
        size = (u64)USER_VADDR_LIMIT - start;
    }
    tag_memmap_region(start, size, MEMMAP_TAG_DIRECT_MAPPED);
}

static void final_arch()
{
    // FIXME: Map uart
    map_range(root_page, PL011_BASE, PL011_BASE, 0x200, 0, 0, 1);
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
}

static void init_arch_mp()
{
    setup_indiv_sp();
    switch_to_el1();
}


/*
 * Init libk
 */
static void init_libk()
{
    init_libk_putchar(raspi3_putchar);
    init_libk_page_table(memmap_palloc, NULL, NULL);
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
        fw_args.fw_name = "none";
        fw_args.fdt.has_supplied = 1;
        fw_args.fdt.supplied = fdt;
    } else {
        fw_args.fw_name = "none";
    }

    // Stack limit
    extern ulong _stack_limit, _stack_limit_mp;
    funcs.stack_limit = (ulong)&_stack_limit;
    funcs.stack_limit_mp = (ulong)&_stack_limit_mp;

    // MP entry
    extern void _start_mp();
    funcs.mp_entry = (ulong)&_start_mp;

    // Prepare funcs
    funcs.init_libk = init_libk;
    funcs.init_arch = init_arch;
    funcs.setup_page = setup_page;
    funcs.map_range = map_range;
    funcs.final_memmap = final_memmap;
    funcs.final_arch = final_arch;
    funcs.jump_to_hal = jump_to_hal;

    // MP funcs
    funcs.init_arch_mp = init_arch_mp;
    funcs.jump_to_hal_mp = jump_to_hal_mp;

    // Go to loader!
    loader(&fw_args, &funcs);
    unreachable();
}

void loader_entry_mp()
{
    kprintf("Loader MP!\n");

    // Go to loader!
    loader_mp();
    unreachable();
}
