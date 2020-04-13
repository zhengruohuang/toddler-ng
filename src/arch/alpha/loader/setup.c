#include "common/include/inttypes.h"
#include "common/include/mach.h"
// #include "common/include/abi.h"
// #include "common/include/msr.h"
// #include "common/include/mem.h"
#include "loader/include/lib.h"
#include "loader/include/loader.h"
#include "loader/include/boot.h"
#include "loader/include/firmware.h"


/*
 * Clipper UART
 */
#define DIRECT_MAP_SEG          (0xfffffc0000000000 + 0x10000000000)
#define SOUTH_BRIDGE_BASE_ADDR  0x1fc000000ull
#define UART_BASE_ADDR          (SOUTH_BRIDGE_BASE_ADDR + 0x3f8)
#define UART_DATA_ADDR          (UART_BASE_ADDR + 0x0)
#define UART_LINE_STAT_ADDR     (UART_BASE_ADDR + 0x5)

static inline u8 mmio_read8(unsigned long addr)
{
    volatile u8 *ptr = (u8 *)(DIRECT_MAP_SEG | addr);
    u8 val = *ptr;
    return val;
}

static inline void mmio_write8(unsigned long addr, u8 val)
{
    volatile u8 *ptr = (u8 *)(DIRECT_MAP_SEG | addr);
    *ptr = val;
}

int arch_debug_putchar(int ch)
{
    u32 ready = 0;
    while (!ready) {
        ready = mmio_read8(UART_LINE_STAT_ADDR) & 0x20;
    }

    mmio_write8(UART_DATA_ADDR, (u8)ch & 0xff);
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
// static struct page_frame *root_page = NULL;
// static struct page_frame *ctxt_table = NULL;
//
// static void *alloc_page()
// {
//     void *page = memmap_alloc_phys(PAGE_SIZE, PAGE_SIZE);
//     return page;
// }

static void *setup_page()
{
//     // Allocate root page
//     root_page = memmap_alloc_phys(PAGE_SIZE, PAGE_SIZE);
//     memzero(root_page, PAGE_SIZE);
//
//     // Allocate and set up ctxt table, we use a entire page for ctxt table
//     ctxt_table = memmap_alloc_phys(PAGE_SIZE, PAGE_SIZE);
//     memzero(ctxt_table, PAGE_SIZE);
//     ctxt_table->pte[0].pfn = ADDR_TO_PFN((ulong)root_page);
//     ctxt_table->pte[0].type = PAGE_TABLE_ENTRY_TYPE_PTD;
//
//     return root_page;
}

// static void map_page(void *page_table, void *vaddr, void *paddr, int block,
//     int cache, int exec, int write)
// {
//     struct page_frame *page = page_table;
//     struct page_table_entry *entry = NULL;
//
//     for (int level = 1; level <= block; level++) {
//         int idx = (int)GET_PTE_INDEX((ulong)vaddr, level);
//         entry = &page->entries[idx];
//
//         if (level != block) {
//             if (!entry->type) {
//                 page = alloc_page();
//                 memzero(page, sizeof(struct page_frame));
//
//                 entry->type = PAGE_TABLE_ENTRY_TYPE_PTD;
//                 entry->pfn = ADDR_TO_PFN((ulong)page);
//             } else {
//                 panic_if(entry->type != PAGE_TABLE_ENTRY_TYPE_PTD,
//                     "Entry type must be PTD: %d\n", entry->type);
//                 page = (void *)PFN_TO_ADDR((ulong)entry->pfn);
//             }
//         }
//     }
//
//     if (entry->type) {
//         panic_if(entry->type != PAGE_TABLE_ENTRY_TYPE_PTE,
//             "Entry type must be PTE: %d\n", entry->type);
//         panic_if((ulong)entry->pfn != ADDR_TO_PFN((ulong)paddr) ||
//             entry->cacheable != cache ||
//             entry->perm != get_pte_perm(1, exec, write),
//             "Trying to change an existing mapping!");
//     }
//
//     else {
//         entry->type = PAGE_TABLE_ENTRY_TYPE_PTE;
//         entry->cacheable = cache;
//         entry->perm = get_pte_perm(1, exec, write);
//         entry->pfn = ADDR_TO_PFN((ulong)paddr);
//     }
// }

static int map_range(void *page_table, void *vaddr, void *paddr, ulong size,
    int cache, int exec, int write)
{
    return 0;

//     kprintf("Mapping %p -> %p, size: %lx, cache: %d, exec: %d, write: %d\n",
//             vaddr, paddr, size, cache, exec, write);
//
//     ulong vaddr_start = ALIGN_DOWN((ulong)vaddr, PAGE_SIZE);
//     ulong paddr_start = ALIGN_DOWN((ulong)paddr, PAGE_SIZE);
//     ulong vaddr_end = ALIGN_UP((ulong)vaddr + size, PAGE_SIZE);
//
//     int mapped_pages = 0;
//
//     for (ulong cur_vaddr = vaddr_start, cur_paddr = paddr_start;
//         cur_vaddr < vaddr_end;
//     ) {
//         // 16MB block
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
//         // 256KB block
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
//             map_page(page_table, (void *)cur_vaddr, (void *)cur_paddr, 3,
//                 cache, exec, write);
//
//             mapped_pages++;
//             cur_vaddr += PAGE_SIZE;
//             cur_paddr += PAGE_SIZE;
//         }
//     }
//
//     return mapped_pages;
}


/*
 * Jump to HAL
 */
typedef void (*hal_start)(struct loader_args *largs);

static void jump_to_hal()
{
    struct loader_args *largs = get_loader_args();
    kprintf("Jump to HAL @ %p\n", largs->hal_entry);

    while (1);

//     enable_mmu();
//
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
 * The Alpha entry point
 */
#define PARAM_OFFSET -0x6000
#define HWRPB_BASE 0x10000000ul

void loader_entry(void *arg1, void *arg2, void *arg3, void *arg4, void *arg5)
{
//     while (1);
    arch_debug_putchar('g');
//     while (1);

    arch_debug_putchar('o');
    arch_debug_putchar('o');
    arch_debug_putchar('d');
    kprintf("Hello, arg1 @ %p, arg2 @ %p, arg3 @ %p, arg4 @ %p, arg5 @ %p\n",
        arg1, arg2, arg3, arg4, arg5);
//     while (1);

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

    // Prepare initrd
    ulong *initrd_start = (void *)(0xfffffc0001010000ull + PARAM_OFFSET + 0x100ull);
    ulong *initrd_size = (void *)(0xfffffc0001010000ull + PARAM_OFFSET + 0x108ull);
    kprintf("initrd_start @ %lx, initrd_size: %lx\n", *initrd_start, *initrd_size);

    // Prepare arg
//     fw_args.type = FW_SRM;
//     fw_args.srm.cmdline = (void *)(0xfffffc0001010000ull + PARAM_OFFSET);
//     fw_args.srm.initrd_start = (void *)*initrd_start;
//     fw_args.srm.initrd_size = *initrd_size;
//     fw_args.srm.hwrpb_base = (void *)HWRPB_BASE;

    struct firmware_params_srm fw_params;
    fw_params.hwrpb_base = (void *)HWRPB_BASE;
    fw_params.cmdline = (void *)(0xfffffc0001010000ull + PARAM_OFFSET);
    fw_params.initrd_start = (void *)*initrd_start;
    fw_params.initrd_size = *initrd_size;

    fw_args.fw_name = "srm";
    fw_args.fw_params = &fw_params;

//     // Prepare arch info
//     // Note here we don't explicit reserve stack because it is directly
//     // declared in start.S
//     funcs.reserved_stack_size = 0;
//     funcs.page_size = PAGE_SIZE;
//     funcs.num_reserved_got_entries = ELF_GOT_NUM_RESERVED_ENTRIES;
//     funcs.phys_mem_range_min = PHYS_MEM_RANGE_MIN;
//     funcs.phys_mem_range_max = PHYS_MEM_RANGE_MAX;

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
