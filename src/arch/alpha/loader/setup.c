#include "common/include/inttypes.h"
#include "common/include/page.h"
#include "loader/include/kprintf.h"
#include "loader/include/lib.h"
#include "loader/include/firmware.h"
#include "loader/include/boot.h"
#include "loader/include/mem.h"
#include "loader/include/loader.h"


/*
 * Direct access helpers
 */
#define SEG_DIRECT_SIZE         0x20000000000ul
#define SEG_DIRECT_MAPPED       0xfffffc0000000000ul
#define SEG_DIRECT_MAPPED_IO    (SEG_DIRECT_MAPPED + 0x10000000000ul)

static inline void *cast_paddr_to_cached_seg(paddr_t paddr)
{
    panic_if(paddr >= (paddr_t)SEG_DIRECT_SIZE,
             "Invalid paddr @ %llx\n", (u64)paddr);
    ulong vaddr = cast_paddr_to_vaddr(paddr) | SEG_DIRECT_MAPPED;
    return (void *)vaddr;
}

static inline void *cast_paddr_to_io_seg(paddr_t paddr)
{
    panic_if(paddr >= (paddr_t)SEG_DIRECT_SIZE,
             "Invalid paddr @ %llx\n", (u64)paddr);
    ulong vaddr = cast_paddr_to_vaddr(paddr) | SEG_DIRECT_MAPPED_IO;
    return (void *)vaddr;
}

static inline paddr_t cast_direct_seg_to_paddr(void *ptr)
{
    ulong vaddr = (ulong)ptr;
    vaddr &= (0x1ul << (MAX_NUM_PADDR_BITS + 1)) - 0x1ul;
    return cast_vaddr_to_paddr(vaddr);
}


/*
 * Clipper UART
 */
#define SOUTH_BRIDGE_BASE_ADDR  0x1fc000000ul
#define UART_BASE_ADDR          (SOUTH_BRIDGE_BASE_ADDR + 0x3f8ul)
#define UART_DATA_ADDR          (UART_BASE_ADDR + 0x0ul)
#define UART_LINE_STAT_ADDR     (UART_BASE_ADDR + 0x5ul)

static inline u8 mmio_read8(unsigned long addr)
{
    volatile u8 *ptr = (u8 *)cast_paddr_to_io_seg(cast_vaddr_to_paddr(addr));
    u8 val = *ptr;
    return val;
}

static inline void mmio_write8(unsigned long addr, u8 val)
{
    volatile u8 *ptr = (u8 *)cast_paddr_to_io_seg(cast_vaddr_to_paddr(addr));
    *ptr = val;
}

static int clipper_putchar(int ch)
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
static paddr_t access_win_to_phys(void *ptr)
{
    // First make sure the vaddr is within the cached seg
    ulong vaddr = (ulong)ptr;
    panic_if((vaddr & SEG_DIRECT_MAPPED) != SEG_DIRECT_MAPPED,
             "Unable to convert from access win @ %p to phys addr, out of range!", ptr);

    // Map to phyical address
    return cast_direct_seg_to_paddr(ptr);
}

static void *phys_to_access_win(paddr_t paddr)
{
    // First make sure the address is within the range
    panic_if(paddr >= (paddr_t)SEG_DIRECT_SIZE,
             "Unable to convert from phys to access win addr, out of range!");

    // Map to cached seg
    return cast_paddr_to_cached_seg(paddr);
}


/*
 * Paging
 */
static struct page_frame *root_page = NULL;

static void *alloc_page()
{
    paddr_t paddr = memmap_alloc_phys(PAGE_SIZE, PAGE_SIZE);
    void *page = cast_paddr_to_cached_seg(paddr);
    return page;
}

static void *setup_page()
{
    root_page = alloc_page();
    memzero(root_page, sizeof(struct page_frame));
    return root_page;
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
            if (!entry->valid) {
                page = alloc_page();
                memzero(page, sizeof(struct page_frame));

                paddr_t page_paddr = cast_direct_seg_to_paddr(page);
                ppfn_t page_ppfn = paddr_to_ppfn(page_paddr);

                entry->valid = 1;
                entry->kernel_read = 1;
                entry->kernel_write = 1;
                entry->no_exec = 0;
                entry->no_read = 0;
                entry->no_write = 0;
                entry->pfn = page_ppfn;
            } else {
                paddr_t page_paddr = ppfn_to_paddr((ppfn_t)entry->pfn);
                page = cast_paddr_to_cached_seg(page_paddr);
            }
        }
    }

    if (entry->valid) {
        panic_if((ppfn_t)entry->pfn != paddr_to_ppfn(paddr),
            "Trying to change an existing mapping from %llx to %llx! pte: %llx",
            (ppfn_t)entry->pfn, paddr_to_ppfn(paddr), entry->value);

        if (exec) entry->no_exec = 0;
        if (write) entry->no_write = 0;
    } else {
        entry->valid = 1;
        entry->kernel_read = 1;
        entry->kernel_write = 1;
        entry->no_exec = !exec;
        entry->no_read = 0;
        entry->no_write = !write;
        entry->pfn = paddr_to_ppfn(paddr);
    }
}

static int map_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size,
                     int cache, int exec, int write)
{
    kprintf("Map, page_dir_pfn: %p, vaddr @ %p, paddr @ %llx, size: %ld, cache: %d, exec: %d, write: %d\n",
            page_table, vaddr, (u64)paddr, size, cache, exec, write);

    ulong vaddr_start = align_down_vaddr(vaddr, PAGE_SIZE);
    paddr_t paddr_start = align_down_paddr(paddr, PAGE_SIZE);
    ulong vaddr_end = align_up_vaddr(vaddr + size, PAGE_SIZE);

    int mapped_pages = 0;

    paddr_t cur_paddr = paddr_start;
    for (ulong cur_vaddr = vaddr_start; cur_vaddr < vaddr_end; ) {
        map_page(page_table, cur_vaddr, cur_paddr, cache, exec, write);
        mapped_pages++;
        cur_vaddr += PAGE_SIZE;
        cur_paddr += PAGE_SIZE;
    }

    return mapped_pages;
}


/*
 * Drivers
 */
static void register_drivers()
{
    REGISTER_FIRMWARE_DRIVER(srm);
}


/*
 * Jump to HAL
 */
typedef void (*hal_start_t)(struct loader_args *largs, int mp);

static void switch_pcb()
{
}

static void jump_to_hal()
{
    struct loader_args *largs = get_loader_args();
    kprintf("Jump to HAL @ %p\n", largs->hal_entry);

    hal_start_t hal = largs->hal_entry;
    hal(largs, 0);
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
    map_range(root_page, UART_BASE_ADDR, UART_BASE_ADDR, 0x100, 0, 0, 1);
}


/*
 * Init arch
 */
static void init_arch()
{

}


/*
 * Init libk
 */
static void init_libk()
{
    init_libk_putchar(clipper_putchar);
}


/*
 * The Alpha entry point
 */
#define PARAM_BASE      (0xfffffc0001010000ul)
#define PARAM_OFFSET    (-0x6000ul)
#define HWRPB_BASE      (0x10000000ul)

void loader_entry(void *arg1, void *arg2, void *arg3, void *arg4, void *arg5)
{
    //while (1);
    clipper_putchar('g');
    clipper_putchar('o');
    clipper_putchar('o');
    clipper_putchar('d');

    init_libk_putchar(clipper_putchar);
    kprintf("Hello, arg1 @ %p, arg2 @ %p, arg3 @ %p, arg4 @ %p, arg5 @ %p\n",
        arg1, arg2, arg3, arg4, arg5);
    //while (1);

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
    void *cmdline_start = (void *)(PARAM_BASE + PARAM_OFFSET);
    ulong *initrd_start = (void *)(PARAM_BASE + PARAM_OFFSET + 0x100ull);
    ulong *initrd_size = (void *)(PARAM_BASE + PARAM_OFFSET + 0x108ull);
    kprintf("initrd_start @ %lx, initrd_size: %lx\n", *initrd_start, *initrd_size);

    // Prepare FW params
    struct firmware_params_srm fw_params;
    fw_params.hwrpb_base = (void *)HWRPB_BASE;
    fw_params.cmdline = cmdline_start;
    fw_params.initrd_start = (void *)*initrd_start;
    fw_params.initrd_size = *initrd_size;

    // Prepare arg
    fw_args.fw_name = "srm";
    fw_args.fw_params = &fw_params;

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
    funcs.register_drivers = register_drivers;

    // Go to loader!
    loader(&fw_args, &funcs);
}
