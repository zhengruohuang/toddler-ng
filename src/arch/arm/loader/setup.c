#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "common/include/page.h"
#include "common/include/abi.h"
#include "common/include/msr.h"
#include "loader/include/kprintf.h"
#include "loader/include/lib.h"
#include "loader/include/mem.h"
#include "loader/include/firmware.h"
#include "loader/include/boot.h"
#include "loader/include/loader.h"
#include "loader/include/devtree.h"


/*
 * Raspi2 UART
 */
#define BCM2835_BASE            (0x3f000000ul)
#define PL011_BASE              (BCM2835_BASE + 0x201000ul)
#define PL011_DR                (PL011_BASE)
#define PL011_FR                (PL011_BASE + 0x18ul)

static inline u8 _raw_mmio_read8(ulong addr)
{
    volatile u8 *ptr = (u8 *)addr;
    u8 val = 0;

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

static int raspi2_putchar(int ch)
{
    // Wait until the UART has an empty space in the FIFO
    u32 ready = 0;
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

static void *alloc_page()
{
    paddr_t paddr = memmap_alloc_phys(PAGE_SIZE, PAGE_SIZE);
    return cast_paddr_to_ptr(paddr);
}

static void *alloc_l1table()
{
    paddr_t paddr = memmap_alloc_phys(L1PAGE_TABLE_SIZE, L1PAGE_TABLE_SIZE);
    return cast_paddr_to_ptr(paddr);
}

static void *alloc_l2table()
{
    void *paddr = alloc_page();
    return paddr;
}

static void *setup_page()
{
    root_page = alloc_l1table();
    memzero(root_page, L1PAGE_TABLE_SIZE);

    return root_page;
}

static void map_page(void *page_table, ulong vaddr, paddr_t paddr, int block,
    int cache, int exec, int write)
{
    struct l1table *l1table = page_table;
    int l1idx = GET_L1PTE_INDEX((ulong)vaddr);
    struct l1page_table_entry *l1entry = &l1table->entries[l1idx];

    // 1-level block mapping
    if (block) {
        if (l1entry->block.present) {
            panic_if((ulong)l1entry->block.bfn != (u32)paddr_to_pbfn(paddr) ||
                l1entry->block.no_exec != !exec ||
                l1entry->block.read_only != !write ||
                l1entry->block.cacheable != cache,
                "Trying to change an existing mapping from BFN %x to %x!",
                l1entry->block.bfn, (u32)paddr_to_pbfn(paddr));
        }

        else {
            l1entry->value = 0;
            l1entry->block.present = 1;
            l1entry->block.bfn = (u32)paddr_to_pbfn(paddr);
            l1entry->block.no_exec = !exec;
            l1entry->block.read_only = !write;
            l1entry->block.user_access = 0;   // Kernel page
            l1entry->block.user_write = 1;    // Kernel page write is determined by read_only
            l1entry->block.cacheable = cache;
            l1entry->block.cache_inner = l1entry->block.cache_outer = 0x1;
                                // write-back, write-allocate when cacheable
                                // See TEX[2], TEX[1:0] and CB
        }
    }

    // 2-level page mapping
    else {
        struct l2table *l2table = NULL;
        if (!l1entry->pte.present) {
            l2table = alloc_l2table();
            memzero(l2table, L2PAGE_TABLE_SIZE);

            l1entry->pte.present = 1;
            l1entry->pte.pfn = (u32)ptr_to_vpfn(l2table);
        } else {
            l2table = vpfn_to_ptr((ulong)l1entry->pte.pfn);
        }

        int l2idx = GET_L2PTE_INDEX((ulong)vaddr);
        struct l2page_table_entry *l2entry = &l2table->entries[l2idx];

        if (l2entry->present) {
            panic_if((ulong)l2entry->pfn != (u32)paddr_to_ppfn(paddr),
                "Trying to change an existing mapping from PFN %x to %x!",
                l2entry->pfn, (u32)paddr_to_ppfn(paddr));

            panic_if((int)l2entry->cacheable != cache,
                "Trying to change cacheable attribute!");

            if (exec) l2entry->no_exec = 0;
            if (write) l2entry->read_only = 0;
        }

        else {
            l2entry->present = 1;
            l2entry->pfn = (u32)paddr_to_ppfn(paddr);
            l2entry->no_exec = !exec;
            l2entry->read_only = !write;
            l2entry->user_access = 0;   // Kernel page
            l2entry->user_write = 1;    // Kernel page write is determined by read_only
            l2entry->cacheable = cache;
            l2entry->cache_inner = l2entry->cache_outer = 0x1;
                                // write-back, write-allocate when cacheable
                                // See TEX[2], TEX[1:0] and CB
        }

        for (int i = 0; i < 3; i++) {
            l2table->entries_dup[i][l2idx].value = l2entry->value;
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


/*
 * Drivers
 */
static void register_drivers()
{
    REGISTER_FIRMWARE_DRIVER(atags);
}


/*
 * Jump to HAL
 */
static void enable_mmu(void *root_page)
{
    // Copy the page table base
    write_trans_tab_base0((ulong)root_page);
    write_trans_tab_base1((ulong)root_page);

    // Enable permission check for domain0
    struct domain_access_ctrl_reg domain;
    read_domain_access_ctrl(domain.value);
    domain.domain0 = 0x1;
    write_domain_access_ctrl(domain.value);

    // Set page table ctrl reg
    struct trans_tab_base_ctrl_reg ttbcr;
    ttbcr.value = 0;
    write_trans_tab_base_ctrl(ttbcr.value);

    // Enable the MMU
    struct sys_ctrl_reg sys_ctrl;
    read_sys_ctrl(sys_ctrl.value);
    sys_ctrl.mmu_enabled = 1;
    write_sys_ctrl(sys_ctrl.value);
}

static void enable_caches_bpred()
{
    struct sys_ctrl_reg sys_ctrl;
    read_sys_ctrl(sys_ctrl.value);

    sys_ctrl.dcache_enabled = 1;
    sys_ctrl.icache_enabled = 1;

    sys_ctrl.bpred_enabled = 1;

    write_sys_ctrl(sys_ctrl.value);
}

static void call_hal(void *entry, struct loader_args *largs, int mp)
{
    ulong sp;

    __asm__ __volatile__ (
        "mov %[sp], sp;"
        : [sp] "=r" (sp)
        :
        : "memory"
    );

    __asm__ __volatile__ (
        // Move to System mode
        "cpsid aif, #0x1f;"

        // Set up stack top
        "mov sp, %[stack];"

        // Pass the argument
        "mov r0, %[largs];"
        "mov r1, %[mp];"

        // Call C
        "mov pc, %[hal];"
        :
        : [hal] "r" (entry), [largs] "r" (largs), [mp] "r" (mp), [stack] "r" (sp)
        : "memory"
    );
}

static void jump_to_hal()
{
    struct loader_args *largs = get_loader_args();
    kprintf("Jump to HAL @ %p, page table @ %p\n",
            largs->hal_entry, largs->page_table);

    enable_mmu(largs->page_table);
    enable_caches_bpred();
    call_hal(largs->hal_entry, largs, 0);
}

static void jump_to_hal_mp()
{
    struct loader_args *largs = get_loader_args();
    kprintf("MP Jump to HAL @ %p, page table @ %p\n",
            largs->hal_entry, largs->page_table);

    enable_mmu(largs->page_table);
    enable_caches_bpred();
    call_hal(largs->hal_entry, largs, 1);
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

static void final_arch()
{
    // FIXME: map PL011
    map_range(root_page, PL011_BASE, PL011_BASE, 0x100, 0, 0, 1);
}


/*
 * Init arch
 */
static void init_arch()
{
}

static void init_arch_mp()
{
}


/*
 * Init libk
 */
static void init_libk()
{
    init_libk_putchar(raspi2_putchar);
}


/*
 * The ARM entry point
 */
void loader_entry(ulong zero, ulong mach_id, void *mach_cfg)
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

//     kprintf("zero: %lx, mach_id: %lx, cfg: %p\n", zero, mach_id, mach_cfg);
//     while (1);

    // Prepare arg
    if (!mach_cfg) {
        fw_args.fw_name = "none";
    } else if (is_fdt_header(mach_cfg)) {
        fw_args.fw_name = "none";
        fw_args.fdt.has_supplied = 1;
        fw_args.fdt.supplied = mach_cfg;
    } else {
        fw_args.fw_name = "atags";
        fw_args.fw_params = mach_cfg;
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
    funcs.register_drivers = register_drivers;

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
