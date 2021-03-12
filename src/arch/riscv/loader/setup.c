#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "common/include/msr.h"
#include "loader/include/lib.h"
#include "loader/include/mem.h"
#include "loader/include/boot.h"
#include "loader/include/loader.h"
#include "loader/include/kprintf.h"
#include "loader/include/firmware.h"


/*
 * RISC-V UART
 */
// Virt @ 0x10000000, SiFive-U @ 0x10013000
#define UART_BASE_ADDR          (0x10000000)
#define UART_DATA_ADDR          (UART_BASE_ADDR + 0x0)
#define UART_LINE_STAT_ADDR     (UART_BASE_ADDR + 0x28)

static inline void _raw_mmio_write8(ulong addr, u8 val)
{
    volatile u8 *ptr = (u8 *)addr;
    atomic_mb();
    *ptr = val;
    atomic_mb();
}

static int riscv_uart_putchar(int ch)
{
//     u32 ready = 0;
//     while (!ready) {
//         ready = mmio_read8(UART_LINE_STAT_ADDR) & 0x20;
//     }

    _raw_mmio_write8(UART_DATA_ADDR, ch & 0xff);
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

static void *setup_page()
{
    root_page = alloc_page();
    memzero(root_page, PAGE_SIZE);

    return root_page;
}

static void check_cacheability(u64 paddr, u64 size, int cache)
{
    u64 dram_start = 0;
    u64 dram_range = get_memmap_range(&dram_start);

    if (cache) {
        panic_if(paddr < dram_start || paddr + size > dram_start + dram_range,
                 "Trying to mark uncacheable memory as cacheable @ %llx, size: %llx",
                 paddr, size);
    } else {
        panic_if(paddr >= dram_start && paddr + size <= dram_start + dram_range,
                 "Trying to mark cacheable memory as uncacheable @ %llx, size: %llx",
                 paddr, size);
    }
}

static int map_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size,
                     int cache, int exec, int write)
{
    kprintf("Map range @ %lx -> %llx, size: %lx\n", vaddr, (u64)paddr, size);

    check_cacheability(paddr, size, cache);
    return generic_map_range(page_table, vaddr, paddr, size, cache, exec, write, 1, 0);
}


/*
 * Jump to HAL
 */
static void enable_mmu(void *root_page)
{
    struct supervisor_addr_trans_prot_reg stap;
    stap.value = 0;

    stap.pfn = paddr_to_ppfn(cast_ptr_to_paddr(root_page));
    stap.asid = 0;
#if (defined(ARCH_RISCV64) && MAX_NUM_VADDR_BITS == 48)
    stap.mode = STAP_MODE_SV48;
#elif (defined(ARCH_RISCV64) && MAX_NUM_VADDR_BITS == 39)
    stap.mode = STAP_MODE_SV39;
#elif (defined(ARCH_RISCV32) && MAX_NUM_VADDR_BITS == 32)
    stap.mode = STAP_MODE_SV32;
#else
    #error "Unsupported arch or max vaddr width!"
#endif

    kprintf("Root page table @ %p\n", root_page);

    atomic_mb();
    write_satp(stap.value);
    atomic_ib();

    kprintf("MMU enabled!\n");
}

typedef void (*hal_start_t)(struct loader_args *largs, int mp);

static void call_hal(struct loader_args *largs)
{
    hal_start_t hal = largs->hal_entry;
    hal(largs, 0);
}

static void jump_to_hal()
{
    struct loader_args *largs = get_loader_args();
    kprintf("Jump to HAL @ %p\n", largs->hal_entry);

    enable_mmu(largs->page_table);
    call_hal(largs);
}


/*
 * Finalize
 */
static void final_memmap()
{
    // FIXME: Reserve memory used by firmware
    claim_memmap_region(0x80000000ull, (u64)LOADER_BASE - 0x80000000ull, MEMMAP_USED);

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
    map_range(root_page, UART_BASE_ADDR, UART_BASE_ADDR, 0x100, 0, 0, 1);
}


/*
 * Init arch
 */
static unsigned char dup_fdt[4096];

static void duplicate_fdt()
{
    // Duplicate FDT to writable memory region
    struct firmware_args *fw_args = get_fw_args();
    if (!strcmp(fw_args->fw_name, "none") && fw_args->fdt.has_supplied) {
        copy_fdt(dup_fdt, fw_args->fdt.supplied, sizeof(dup_fdt));
        fw_args->fdt.supplied = dup_fdt;
    }
}

static void setup_pmp()
{
    // Set up a PMP to permit access to all of memory.
    // Ignore the illegal-instruction trap if PMPs aren't supported.
    struct phys_mem_prot_cfg_reg pmpcfg;
    pmpcfg.value = 0;
    pmpcfg.fields[0].read = 1;
    pmpcfg.fields[0].write = 1;
    pmpcfg.fields[0].exec = 1;
    pmpcfg.fields[0].match = PMP_CFG_MATCH_NAPOT;

    ulong tmp = 0;
    //ulong cfg = PMP_NAPOT | PMP_R | PMP_W | PMP_X;

    __asm__ __volatile__ (
        "   la %[tmp], 1f;"
        "   csrrw %[tmp], mtvec, %[tmp];"
        "   csrw pmpaddr0, %[addr];"
        "   csrw pmpcfg0, %[cfg];"
        ".align 2;"
        "1: csrw mtvec, %[tmp];"
        : [tmp] "=&r" (tmp)
        : [cfg] "r" (pmpcfg.value), [addr] "r" (-1ul)
        :
    );
}

static void enter_supervisor_mode()
{
    struct status_reg mstatus;
    read_mstatus(mstatus.value);
    mstatus.mpp = 1;
    mstatus.mpie = 0;
    write_mstatus(mstatus.value);

    ulong tmp = 0;

    __asm__ __volatile__ (
        "   la %[tmp], 1f;"
        "   csrw mepc, %[tmp];"
        "   mret;"
        ".align 2;"
        "1: nop;"
        : [tmp] "=&r" (tmp)
        :
    );
}

static int in_m_mode()
{
    return 0;
}

static void init_arch()
{
    if (in_m_mode()) {
        setup_pmp();
        enter_supervisor_mode();
        duplicate_fdt();
    }
}


/*
 * Init libk
 */
static void init_libk()
{
    init_libk_putchar(riscv_uart_putchar);
    init_libk_page_table(memmap_palloc, NULL, NULL);
}


/*
 * The RISC-V entry point
 */
void loader_entry(ulong boot_hart, void *fdt)
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
    if (fdt) {
        fw_args.fdt.has_supplied = 1;
        fw_args.fdt.supplied = fdt;
    }

    // Prepare funcs
    funcs.init_libk = init_libk;
    funcs.init_arch = init_arch;
    funcs.setup_page = setup_page;
    funcs.map_range = map_range;
    funcs.final_memmap = final_memmap;
    funcs.final_arch = final_arch;
    funcs.jump_to_hal = jump_to_hal;

    // Go to loader!
    loader(&fw_args, &funcs);
}

void loader_entry_ap()
{
}
