#include "common/include/inttypes.h"
#include "common/include/page.h"
#include "common/include/abi.h"
#include "common/include/io.h"
#include "common/include/msr.h"
#include "loader/include/lib.h"
#include "loader/include/mem.h"
#include "loader/include/boot.h"
#include "loader/include/loader.h"
#include "loader/include/firmware.h"
#include "loader/include/kprintf.h"


/*
 * Malta UART
 */
#define SOUTH_BRIDGE_BASE_ADDR  0x18000000
#define UART_BASE_ADDR          (SOUTH_BRIDGE_BASE_ADDR + 0x3f8)
#define UART_DATA_ADDR          (UART_BASE_ADDR + 0x0)
#define UART_LINE_STAT_ADDR     (UART_BASE_ADDR + 0x28)

static int malta_putchar(int ch)
{
    u32 ready = 0;
    while (!ready) {
        ready = mmio_read8(UART_LINE_STAT_ADDR) & 0x20;
    }

    mmio_write8(UART_DATA_ADDR, (u8)ch & 0xff);
    return 1;
}


/*
 * Alignment helpers
 */
static inline void *cast_paddr_to_cached_seg(paddr_t paddr)
{
    paddr_t cached_seg = paddr | (paddr_t)SEG_DIRECT_CACHED;
    return cast_paddr_to_ptr(cached_seg);
}

static inline ulong cast_paddr_to_uncached_seg(paddr_t paddr)
{
    paddr_t cached_seg = paddr | (paddr_t)SEG_DIRECT_UNCACHED;
    return cast_paddr_to_vaddr(cached_seg);
}

static inline paddr_t cast_cached_seg_to_paddr(void *ptr)
{
    ulong vaddr = (ulong)ptr;
    ulong lower = vaddr & (ulong)SEG_DIRECT_MASK;
    return cast_vaddr_to_paddr(lower);
}

static inline paddr_t cast_uncached_seg_to_paddr(ulong vaddr)
{
    ulong lower = vaddr & (ulong)SEG_DIRECT_MASK;
    return cast_vaddr_to_paddr(lower);
}


/*
 * Access window <--> physical addr
 */
static paddr_t access_win_to_phys(void *ptr)
{
    // First make sure the vaddr is within the cached seg
    ulong vaddr = (ulong)ptr;
    panic_if(vaddr < SEG_DIRECT_CACHED || vaddr >= SEG_KERNEL,
        "Unable to convert from access win to phys addr, out of range!");

    // Map to phyical address
    paddr_t paddr = cast_cached_seg_to_paddr(ptr);
    return paddr;
}

static void *phys_to_access_win(paddr_t paddr)
{
    // First make sure the address is within the lower 512MB
    ulong vaddr = cast_paddr_to_vaddr(paddr);
    panic_if(vaddr >= SEG_DIRECT_SIZE,
        "Unable to convert from phys to access win addr, out of range!");

    // Map to cached seg
    return cast_paddr_to_cached_seg(paddr);
}


/*
 * Paging
 */
static void *alloc_page()
{
    paddr_t paddr = memmap_alloc_phys(PAGE_SIZE, PAGE_SIZE);
    return cast_paddr_to_cached_seg(paddr);
}

static void *setup_page()
{
    struct page_frame *root_page = alloc_page();
    memzero(root_page, sizeof(struct page_frame));

    return root_page;
}

static int get_page_table_index(ulong vaddr, int level)
{
    vaddr >>= PAGE_BITS;

    for (int shift = PAGE_LEVELS - 1 - level; shift > 0; shift--) {
        vaddr >>= PAGE_TABLE_ENTRY_BITS;
    }

    return vaddr & ((ulong)PAGE_TABLE_ENTRY_COUNT - 0x1ul);
}

static void map_page(void *page_table, ulong vaddr, paddr_t paddr,
    int cache, int exec, int write)
{
    struct page_frame *page = page_table;
    struct page_table_entry *entry = NULL;

    for (int level = 0; level < PAGE_LEVELS; level++) {
        int idx = get_page_table_index(vaddr, level);
        entry = &page->entries[idx];

        if (level != PAGE_LEVELS - 1) {
            if (!entry->present) {
                page = alloc_page();
                memzero(page, sizeof(struct page_frame));

                paddr_t page_paddr = cast_cached_seg_to_paddr(page);
                ppfn_t page_ppfn = paddr_to_ppfn(page_paddr);

                entry->present = 1;
                entry->pfn = page_ppfn; //ACC_DATA_TO_PHYS(ADDR_TO_PFN((ulong)page));
            } else {
                paddr_t page_paddr = ppfn_to_paddr((ppfn_t)entry->pfn);
                page = cast_paddr_to_cached_seg(page_paddr);
            }
        }
    }

    if (entry->present) {
        panic_if((ppfn_t)entry->pfn != paddr_to_ppfn(paddr),
            "Trying to change an existing mapping!");

        panic_if((int)entry->cache_allow != cache,
            "Trying to change cacheable attribute!");

        if (exec) entry->exec_allow = 1;
        if (write) entry->write_allow = 1;
    } else {
        entry->present = 1;
        entry->cache_allow = cache;
        entry->exec_allow = exec;
        entry->read_allow = 1;
        entry->write_allow = write;
        entry->pfn = (u32)paddr_to_ppfn(paddr);
    }
}

static int map_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size,
                     int cache, int exec, int write)
{
    int mapped_pages = 0;

    ulong vaddr_start = align_down_vaddr(vaddr, PAGE_SIZE);
    ulong vaddr_end = align_up_vaddr(vaddr + size, PAGE_SIZE);

    paddr_t paddr_start = align_down_paddr(paddr, PAGE_SIZE);
    paddr_t cur_paddr = paddr_start;

    for (ulong cur_vaddr = vaddr_start; cur_vaddr < vaddr_end;
         cur_vaddr += PAGE_SIZE, cur_paddr += PAGE_SIZE
    ) {
        if (cur_vaddr < SEG_DIRECT_CACHED && cur_vaddr >= SEG_KERNEL) {
            map_page(page_table, cur_vaddr, cur_paddr, cache, exec, write);
            mapped_pages++;
        }
    }

    kprintf("mapped_pages: %d, start @ %lx, end @ %lx\n", mapped_pages, vaddr_start, vaddr_end);
    return mapped_pages;
}


/*
 * Jump to HAL
 */
typedef void (*hal_start)(struct loader_args *largs, int mp);

static void jump_to_hal()
{
    struct loader_args *largs = get_loader_args();
    kprintf("Jump to HAL @ %p\n", largs->hal_entry);

    hal_start hal = largs->hal_entry;
    hal(largs, 0);
}


/*
 * Finalize
 */
#define DIRECT_ACCESS_END 0x20000000ull
#define DIRECT_MAPPED_END 0x80000000ull

static void final_memmap()
{
    u64 start = 0;
    u64 size = get_memmap_range(&start);
    u64 end = start + size;

    panic_if(start >= DIRECT_ACCESS_END,
             "MIPS must have a direct access region!\n");

    // Mark the lower 2GB as direct mapped
    u64 direct_mapped_size = size;
    if (end > DIRECT_MAPPED_END) direct_mapped_size = DIRECT_MAPPED_END - start;
    tag_memmap_region(start, size, MEMMAP_TAG_DIRECT_MAPPED);

    // Mark the lower 512MB as direct access
    u64 direct_access_size = size;
    if (end > DIRECT_ACCESS_END) direct_access_size = DIRECT_ACCESS_END - start;
    tag_memmap_region(start, size, MEMMAP_TAG_DIRECT_ACCESS);
}

static void final_arch()
{
}


/*
 * Init arch
 */
static void init_arch()
{
    struct cp0_status reg;

    // Read CP0_STATUS
    read_cp0_status(reg.value);

    // Enable 64-bit segments
#if (ARCH_WIDTH == 64)
    reg.kx = 1;
    reg.sx = 1;
    reg.ux = 1;
    reg.px = 1;
#endif

    // Disable interrupts
    reg.ie = 0;
    reg.ksu = 0;
    reg.exl = 0;
    reg.erl = 0;

    // Apply to CP0_STATUS
    write_cp0_status(reg.value);
}


/*
 * Init libk
 */
static void init_libk()
{
    init_libk_putchar(malta_putchar);
}


/*
 * The MIPS entry point
 */
void loader_entry(int kargc, char **kargv, char **env, ulong mem_size)
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
    struct firmware_params_karg karg_params;

    if (kargc == -2) {
        fw_args.fw_name = "none";
        fw_args.fdt.has_supplied = 1;
        fw_args.fdt.supplied = (void *)kargv;
    } else {
        fw_args.fw_name = "karg";
        fw_args.fw_params = &karg_params;
        karg_params.kargc = kargc;
        karg_params.kargv = kargv;
        karg_params.env = env;
        karg_params.mem_size = mem_size;
    }

    // Prepare funcs
    funcs.init_libk = init_libk;
    funcs.init_arch = init_arch;
    funcs.setup_page = setup_page;
    funcs.map_range = map_range;
    funcs.has_direct_access = 1;
    funcs.access_win_to_phys = access_win_to_phys;
    funcs.phys_to_access_win = phys_to_access_win;
    funcs.final_memmap = final_memmap;
    funcs.final_arch = final_arch;
    funcs.jump_to_hal = jump_to_hal;

    // Go to loader!
    loader(&fw_args, &funcs);
}
