#include "common/include/inttypes.h"
#include "common/include/io.h"
#include "common/include/mem.h"
#include "common/include/abi.h"
#include "common/include/msr.h"
#include "loader/include/lib.h"
#include "loader/include/firmware.h"
#include "loader/include/boot.h"
#include "loader/include/loader.h"


/*
 * Raspi2 UART
 */
#define BCM2835_BASE            (0x3f000000ul)
#define PL011_BASE              (BCM2835_BASE + 0x201000ul)
#define PL011_DR                (PL011_BASE)
#define PL011_FR                (PL011_BASE + 0x18ul)

int arch_debug_putchar(int ch)
{
    // Wait until the UART has an empty space in the FIFO
    u32 ready = 0;
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

static void *alloc_l1table()
{
    void *paddr = memmap_alloc_phys(L1PAGE_TABLE_SIZE, L1PAGE_TABLE_SIZE);
    return (void *)((ulong)paddr);
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

static void map_page(void *page_table, void *vaddr, void *paddr, int block,
    int cache, int exec, int write)
{
    struct l1table *l1table = page_table;
    int l1idx = GET_L1PTE_INDEX((ulong)vaddr);
    struct l1page_table_entry *l1entry = &l1table->entries[l1idx];

    // 1-level block mapping
    if (block) {
        if (l1entry->block.present) {
            panic_if((ulong)l1entry->block.bfn != ADDR_TO_BFN((ulong)paddr) ||
                l1entry->block.no_exec != !exec ||
                l1entry->block.read_only != !write ||
                l1entry->block.cacheable != cache,
                "Trying to change an existing mapping from BFN %lx to %lx!",
                (ulong)l1entry->block.bfn, ADDR_TO_BFN((ulong)paddr));
        }

        else {
            l1entry->block.present = 1;
            l1entry->block.bfn = ADDR_TO_BFN((ulong)paddr);
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
            l1entry->pte.pfn = ADDR_TO_PFN((ulong)l2table);
        } else {
            l2table = (void *)PFN_TO_ADDR((ulong)l1entry->pte.pfn);
        }

        int l2idx = GET_L2PTE_INDEX((ulong)vaddr);
        struct l2page_table_entry *l2entry = &l2table->entries[l2idx];

        if (l2entry->present) {
            panic_if((ulong)l2entry->pfn != ADDR_TO_PFN((ulong)paddr),
                "Trying to change an existing mapping from PFN %lx to %lx!",
                (ulong)l2entry->pfn, ADDR_TO_PFN((ulong)paddr));

            panic_if((int)l2entry->cacheable != cache,
                "Trying to change cacheable attribute!");

            if (exec) l2entry->no_exec = 0;
            if (write) l2entry->read_only = 0;
        }

        else {
            l2entry->present = 1;
            l2entry->pfn = ADDR_TO_PFN((ulong)paddr);
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

static int map_range(void *page_table, void *vaddr, void *paddr, ulong size,
    int cache, int exec, int write)
{
    ulong vaddr_start = ALIGN_DOWN((ulong)vaddr, PAGE_SIZE);
    ulong paddr_start = ALIGN_DOWN((ulong)paddr, PAGE_SIZE);
    ulong vaddr_end = ALIGN_UP((ulong)vaddr + size, PAGE_SIZE);

    int mapped_pages = 0;

    for (ulong cur_vaddr = vaddr_start, cur_paddr = paddr_start;
        cur_vaddr < vaddr_end;
    ) {
        // 1MB block
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

        // 4KB page
        else {
            map_page(page_table, (void *)cur_vaddr, (void *)cur_paddr, 0,
                cache, exec, write);

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
static void enable_mmu(void *root_page)
{
    // FIXME: map PL011
    map_range(root_page, (void *)PL011_BASE, (void *)PL011_BASE, 0x100, 0, 0, 1);

    // Copy the page table address to cp15
    write_trans_tab_base0((ulong)root_page);

    // Enable permission check for domain0
    struct domain_access_ctrl_reg domain;
    read_domain_access_ctrl(domain.value);
    domain.domain0 = 0x1;
    write_domain_access_ctrl(domain.value);

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

static void call_hal(void *entry, struct loader_args *largs)
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

        // Call C
        "mov pc, %[hal];"
        :
        : [hal] "r" (entry), [largs] "r" (largs), [stack] "r" (sp)
        : "memory"
    );
}

static void jump_to_hal()
{
    struct loader_args *largs = get_loader_args();
    lprintf("Jump to HAL @ %p\n", largs->hal_entry);

    enable_mmu(largs->page_table);
    enable_caches_bpred();
    call_hal(largs->hal_entry, largs);

//     hal_start hal = largs->hal_entry;
//     hal(largs);
}


/*
 * Init arch
 */
static void init_arch()
{

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

//     lprintf("zero: %lx, mach_id: %lx, cfg: %p\n", zero, mach_id, mach_cfg);
//     while (1);

    // Prepare arg
    if (!mach_cfg) {
        fw_args.type = FW_NONE;
    } else if (is_fdt_header(mach_cfg)) {
        fw_args.type = FW_FDT;
        fw_args.fdt.fdt = mach_cfg;
    } else {
        fw_args.type = FW_ATAGS;
        fw_args.atags.atags = mach_cfg;
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
