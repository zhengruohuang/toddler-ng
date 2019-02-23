#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "common/include/io.h"
#include "common/include/msr.h"
#include "loader/include/lib.h"
#include "loader/include/boot.h"
#include "loader/include/loader.h"


/*
 * Malta UART
 */
#define SOUTH_BRIDGE_BASE_ADDR  0x18000000
#define UART_BASE_ADDR          (SOUTH_BRIDGE_BASE_ADDR + 0x3f8)
#define UART_DATA_ADDR          (UART_BASE_ADDR + 0x0)
#define UART_LINE_STAT_ADDR     (UART_BASE_ADDR + 0x28)

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
    ulong addr = (ulong)vaddr;
    panic_if(addr < SEG_ACC_CACHED || addr >= SEG_KERNEL,
        "Unable to convert from access win to phys addr, out of range!");

    addr = ACC_DATA_TO_PHYS(addr);
    return (void *)addr;
}

static void *phys_to_access_win(void *paddr)
{
    ulong addr = (ulong)paddr;
    panic_if(addr >= SEG_ACC_SIZE,
        "Unable to convert from phys to access win addr, out of range!");

    addr = PHYS_TO_ACC_DATA(addr);
    return (void *)addr;
}


/*
 * Paging
 */
static void *alloc_page()
{
    void *paddr = memmap_alloc_phys(PAGE_SIZE, PAGE_SIZE);
    return (void *)(PHYS_TO_ACC_DATA((ulong)paddr));
}

static void *setup_page()
{
    struct page_frame *root_page = alloc_page();
    memzero(root_page, sizeof(struct page_frame));

    return root_page;
}

static int get_page_table_index(void *vaddr, int level)
{
    ulong addr = (ulong)vaddr;
    addr >>= PAGE_BITS;

    for (int shift = PAGE_LEVELS - 1 - level; shift > 0; shift--) {
        addr >>= PAGE_TABLE_ENTRY_BITS;
    }

    return addr & ((ulong)PAGE_TABLE_ENTRY_COUNT - 0x1ul);
}

static void map_page(void *page_table, void *vaddr, void *paddr,
    int cache, int exec, int read, int write)
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

                entry->present = 1;
                entry->pfn = ACC_DATA_TO_PHYS(ADDR_TO_PFN((ulong)page));
            } else {
                page = (void *)PHYS_TO_ACC_DATA(PFN_TO_ADDR((ulong)entry->pfn));
            }
        }
    }

    if (entry->present) {
        panic_if((ulong)entry->pfn != ADDR_TO_PFN((ulong)paddr),
            "Trying to change an existing mapping!");

        panic_if((int)entry->cache_allow != cache,
            "Trying to change cacheable attribute!");

        if (exec) entry->exec_allow = 1;
        if (read) entry->read_allow = 1;
        if (write) entry->write_allow = 1;
    } else {
        entry->present = 1;
        entry->cache_allow = cache;
        entry->exec_allow = exec;
        entry->read_allow = read;
        entry->write_allow = write;
        entry->pfn = ADDR_TO_PFN((ulong)paddr);
    }
}

static int map_range(void *page_table, void *vaddr, void *paddr, ulong size,
    int cache, int exec, int read, int write)
{
    ulong vaddr_start = ALIGN_DOWN((ulong)vaddr, PAGE_SIZE);
    ulong paddr_start = ALIGN_DOWN((ulong)paddr, PAGE_SIZE);
    ulong vaddr_end = ALIGN_UP((ulong)vaddr + size, PAGE_SIZE);

    int mapped_pages = 0;

    for (ulong cur_vaddr = vaddr_start, cur_paddr = paddr_start;
        cur_vaddr < vaddr_end; cur_vaddr += PAGE_SIZE, cur_paddr += PAGE_SIZE
    ) {
        if (cur_vaddr < SEG_LOW_CACHED && cur_vaddr >= SEG_KERNEL) {
            map_page(page_table, (void *)cur_vaddr, (void *)cur_paddr,
                cache, exec, read, write);
            mapped_pages++;
        }
    }

    return mapped_pages;
}


/*
 * Jump to HAL
 */
typedef void (*hal_start)(struct loader_args *largs);

static void jump_to_hal()
{
    struct loader_args *largs = get_loader_args();
    lprintf("Jump to HAL @ %p\n", largs->hal_entry);

    hal_start hal = largs->hal_entry;
    hal(largs);
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
    if (kargc == -2) {
        fw_args.type = FW_FDT;
        fw_args.fdt.fdt = (void *)kargv;
    } else {
        fw_args.type = FW_KARG;
        fw_args.karg.kargc = kargc;
        fw_args.karg.kargv = kargv;
        fw_args.karg.env = env;
        fw_args.karg.mem_size = mem_size;
    }

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
