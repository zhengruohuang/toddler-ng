#include "common/include/inttypes.h"
#include "common/include/abi.h"
#include "common/include/msr.h"
#include "common/include/page.h"
#include "loader/include/lib.h"
#include "loader/include/loader.h"
#include "loader/include/boot.h"


#if (defined(MACH_LEON3))
/*
 * LEON3 UART
 */
#define APBUART_BASE    (0x80000100ul)
#define APBUART_DATA    (APBUART_BASE + 0)
#define APBUART_CTRL    (APBUART_BASE + 0x8ul)

#define APBUART_TRANSMIT_ENABLED (0x1ul << 1)

int arch_debug_putchar(int ch)
{
    // Enable UART if needed
    u32 ctrl = 0;
    asi_read32(ASI_MMU_BYPASS, APBUART_CTRL, ctrl);
    if (!(ctrl & APBUART_TRANSMIT_ENABLED)) {
        ctrl |= APBUART_TRANSMIT_ENABLED;
        asi_write32(ASI_MMU_BYPASS, APBUART_CTRL, ctrl);
    }

    // Write to UART
    asi_write32(ASI_MMU_BYPASS, APBUART_DATA, ch);
    return 1;
}
#else
/*
 * OBP
 */
#include "loader/include/obp.h"

static struct openboot_prom *debug_obp;

static void init_debug_obp()
{
    struct firmware_args *fw_args = get_fw_args();
    debug_obp = fw_args->obp.obp;
}

int arch_debug_putchar(int ch)
{
    if (debug_obp) {
        char s[2] = { (char)ch, '\0' };
        debug_obp->putstr(s, 2);
    }

}
#endif


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
static struct page_frame *ctxt_table = NULL;

static void *alloc_page()
{
    void *page = memmap_alloc_phys(PAGE_SIZE, PAGE_SIZE);
    return page;
}

static void *setup_page()
{
    // Allocate root page
    root_page = memmap_alloc_phys(PAGE_SIZE, PAGE_SIZE);
    memzero(root_page, PAGE_SIZE);

    // Allocate and set up ctxt table, we use a entire page for ctxt table
    ctxt_table = memmap_alloc_phys(PAGE_SIZE, PAGE_SIZE);
    memzero(ctxt_table, PAGE_SIZE);
    ctxt_table->pte[0].pfn = ADDR_TO_PFN((ulong)root_page);
    ctxt_table->pte[0].type = PAGE_TABLE_ENTRY_TYPE_PTD;

    return root_page;
}

static int get_pte_perm(int super, int exec, int write)
{
    super = !!super;
    exec = !!exec;
    write = !!write;

    int mask = (super << 2) | (exec << 1) | write;

    switch (mask) {
    // Supervisor
    case 7: return 7;   // Super + Exec + Write
    case 6: return 6;   // Super + Exec only
    case 5: return 7;   // Super + Write only -> Super + Exec + Write
    case 4: return 6;   // Super + Read only -> Super + Exec

    // User
    case 3: return 3;   // Exec + Write
    case 2: return 4;   // Exec only
    case 1: return 1;   // Write only
    case 0: return 0;   // Read only

    default: return 0;
    }

    return 0;
}

static void map_page(void *page_table, void *vaddr, void *paddr, int block,
    int cache, int exec, int write)
{
    struct page_frame *page = page_table;
    struct page_table_entry *entry = NULL;

    for (int level = 1; level <= block; level++) {
        int idx = (int)GET_PTE_INDEX((ulong)vaddr, level);
        entry = &page->entries[idx];

        if (level != block) {
            if (!entry->type) {
                page = alloc_page();
                memzero(page, sizeof(struct page_frame));

                entry->type = PAGE_TABLE_ENTRY_TYPE_PTD;
                entry->pfn = ADDR_TO_PFN((ulong)page);
            } else {
                panic_if(entry->type != PAGE_TABLE_ENTRY_TYPE_PTD,
                    "Entry type must be PTD: %d\n", entry->type);
                page = (void *)PFN_TO_ADDR((ulong)entry->pfn);
            }
        }
    }

    if (entry->type) {
        panic_if(entry->type != PAGE_TABLE_ENTRY_TYPE_PTE,
            "Entry type must be PTE: %d\n", entry->type);
        panic_if((ulong)entry->pfn != ADDR_TO_PFN((ulong)paddr) ||
            entry->cacheable != cache ||
            entry->perm != get_pte_perm(1, exec, write),
            "Trying to change an existing mapping!");
    }

    else {
        entry->type = PAGE_TABLE_ENTRY_TYPE_PTE;
        entry->cacheable = cache;
        entry->perm = get_pte_perm(1, exec, write);
        entry->pfn = ADDR_TO_PFN((ulong)paddr);
    }
}

static int map_range(void *page_table, void *vaddr, void *paddr, ulong size,
    int cache, int exec, int write)
{
    kprintf("Mapping %p -> %p, size: %lx, cache: %d, exec: %d, write: %d\n",
            vaddr, paddr, size, cache, exec, write);

    ulong vaddr_start = ALIGN_DOWN((ulong)vaddr, PAGE_SIZE);
    ulong paddr_start = ALIGN_DOWN((ulong)paddr, PAGE_SIZE);
    ulong vaddr_end = ALIGN_UP((ulong)vaddr + size, PAGE_SIZE);

    int mapped_pages = 0;

    for (ulong cur_vaddr = vaddr_start, cur_paddr = paddr_start;
        cur_vaddr < vaddr_end;
    ) {
        // 16MB block
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

        // 256KB block
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
            map_page(page_table, (void *)cur_vaddr, (void *)cur_paddr, 3,
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
typedef void (*hal_start)(struct loader_args *largs);

static void enable_mmu()
{
    // Set up context table registers
    struct mmu_ctxt_table mmu_ctr;
    mmu_ctr.value = 0;
    mmu_ctr.ctxt_table_pfn = ADDR_TO_PFN((ulong)ctxt_table);
    write_mmu_ctxt_table(mmu_ctr.value);
    write_mmu_ctxt_num(0);

    // Enable MMU
    struct mmu_ctrl mmu_cr;
    read_mmu_ctrl(mmu_cr.value);
    mmu_cr.enabled = 1;
    write_mmu_ctrl(mmu_cr.value);
}

static void jump_to_hal()
{
    struct loader_args *largs = get_loader_args();
    kprintf("Jump to HAL @ %p\n", largs->hal_entry);

    enable_mmu();

    hal_start hal = largs->hal_entry;
    hal(largs);
}


/*
 * Init arch
 */
static void test_reg_win(int level)
{
    kprintf("Enter level: %d\n", level);

    if (level < 32) {
        test_reg_win(level + 1);
    }

    kprintf("Leave level: %d\n", level);
}

static void init_arch()
{
#if defined(MACH_SUN4M)
    init_debug_obp();

    arch_debug_putchar('g');
    arch_debug_putchar('o');
    arch_debug_putchar('o');
    arch_debug_putchar('d');
    arch_debug_putchar('\n');
    kprintf("Test: %d, %d, %d, %d, %d\n", 1, 2, 3, 4, 5);
    kprintf("Test: %s\n", "test");
//     while (1);
#endif

    test_reg_win(0);
}


/*
 * The SPARCv8 entry point
 */
void loader_entry(void *obp_entry)
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
    if (obp_entry) {
        fw_args.fw_name = "obp";
        fw_args.fw_params = obp_entry;
    } else {
        fw_args.fw_name = "none";
    }

    // Prepare arch info
    // Note here we don't explicit reserve stack because it is directly
    // declared in start.S
    //funcs.reserved_stack_size = 0;
    //funcs.page_size = PAGE_SIZE;
    //funcs.num_reserved_got_entries = ELF_GOT_NUM_RESERVED_ENTRIES;
    //funcs.phys_mem_range_min = PHYS_MEM_RANGE_MIN;
    //funcs.phys_mem_range_max = PHYS_MEM_RANGE_MAX;

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
