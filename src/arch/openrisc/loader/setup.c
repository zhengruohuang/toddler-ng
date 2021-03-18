#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "common/include/msr.h"
#include "loader/include/lib.h"
#include "loader/include/mem.h"
#include "loader/include/boot.h"
#include "loader/include/loader.h"
#include "loader/include/kprintf.h"


/*
 * Dummy UART
 */
static inline void _raw_mmio_write8(ulong addr, u8 val)
{
    volatile u8 *ptr = (u8 *)(addr);
    *ptr = val;
    atomic_mb();
}

#define UART_BASE_ADDR          (0x90000000)
#define UART_DATA_ADDR          (UART_BASE_ADDR + 0x0)

static int or1k_putchar(int ch)
{
    _raw_mmio_write8(UART_DATA_ADDR, (u8)ch & 0xff);
    return 1;
}


/*
 * Access window <--> physical addr
 */
static paddr_t access_win_to_phys(void *ptr)
{
    return cast_ptr_to_paddr(ptr);
}

static void *phys_to_access_win(paddr_t paddr)
{
    return cast_paddr_to_ptr(paddr);
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

static int map_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size,
                     int cache, int exec, int write)
{
    return generic_map_range(page_table, vaddr, paddr, size, cache, exec, write, 1, 0);
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

    panic("Loader encountered except #%d: %s, EPC @ %lx, EEAR @ %lx\n",
          code, ename, epc, eear);
    while (1);
}


/*
 * Jump to HAL
 */
typedef void (*hal_start_t)(struct loader_args *largs, int mp);

static void call_hal(struct loader_args *largs, int mp)
{
    hal_start_t hal = largs->hal_entry;
    hal(largs, mp);
}

static void jump_to_hal()
{
    struct loader_args *largs = get_loader_args();
    kprintf("Jump to HAL @ %p\n", largs->hal_entry);

    call_hal(largs, 0);
}

static void jump_to_hal_mp()
{
    struct loader_args *largs = get_loader_args();
    kprintf("MP Jump to HAL @ %p\n", largs->hal_entry);

    call_hal(largs, 1);
}


/*
 * Finalize
 */
static void final_memmap()
{
    // All physical memory is direct mapped
    u64 start = 0;
    u64 size = get_memmap_range(&start);
    tag_memmap_region(start, size, MEMMAP_TAG_DIRECT_MAPPED);
}

static void disable_mmu_and_caches()
{
    struct supervision_reg sr;
    read_sr(sr.value);

    sr.dmmu_enabled = 0;
    sr.immu_enabled = 0;
    sr.dcache_enabled = 0;
    sr.icache_enabled = 0;

    write_sr(sr.value);
}

static void final_arch()
{
    // FIXME: map UART
    map_range(root_page, UART_BASE_ADDR, UART_BASE_ADDR, 0x100, 0, 0, 1);

    // Override sysarea
    struct loader_args *largs = get_loader_args();
    largs->sysarea_grows_up = 1;
    largs->sysarea_lower = 0xfff00000;
    largs->sysarea_upper = 0xfff00000;
}


/*
 * Init
 */
static void check_features()
{
    struct cpu_config_reg cpucfgr;
    read_cpucfgr(cpucfgr.value);

    // O1RK port requires the CPU to have at least two sets of shadow registers
    // for exception handling
    panic_if(cpucfgr.num_shadow_gpr_files < 2,
             "OR1K port requires at least two sets of shadow registers!\n");

    // O1RK port requires the CPU to support except base reg, EVBAR
    // for exception handling
    panic_if(!cpucfgr.has_except_base_reg,
             "OR1K port requres except base reg EVBAR!\n");

    struct unit_present_reg upr;
    read_upr(upr.value);
    panic_if(!upr.valid || !upr.immu || !upr.dmmu, "OR1K port requres MMUs!\n");
}

static void init_arch()
{
    disable_mmu_and_caches();
    check_features();
}

static void init_libk()
{
    init_libk_putchar(or1k_putchar);
    init_libk_page_table(memmap_palloc, NULL, phys_to_access_win);
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

    // MP entry
    extern ulong _start_mp_flag;
    funcs.mp_entry = (ulong)&_start_mp_flag;

    // Prepare funcs
    funcs.init_libk = init_libk;
    funcs.init_arch = init_arch;
    funcs.setup_page = setup_page;
    funcs.map_range = map_range;
    funcs.final_memmap = final_memmap;
    funcs.final_arch = final_arch;
    funcs.jump_to_hal = jump_to_hal;

    // OR1K loader pretends to have direct access by keeping MMU disabled,
    // so that loader won't need to deal with TLB misses
    funcs.has_direct_access = 1;
    funcs.phys_to_access_win = phys_to_access_win;
    funcs.access_win_to_phys = access_win_to_phys;

    // Go to loader!
    loader(&fw_args, &funcs);

    // Should never reach here
    panic("Should never reach here");
    while (1);
}

void loader_entry_mp()
{
    ulong coreid = 0;
    read_core_id(coreid);
    kprintf("Booting hart: %lx\n", coreid);
    kprintf("Loader MP!\n");

    // Go to HAL!
    jump_to_hal_mp();

    // Should never reach here
    panic("Should never reach here");
    while (1);
}
