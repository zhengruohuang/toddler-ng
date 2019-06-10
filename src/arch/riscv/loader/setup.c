#include "common/include/inttypes.h"
// #include "common/include/io.h"
#include "common/include/mem.h"
// #include "common/include/abi.h"
#include "common/include/msr.h"
#include "loader/include/lib.h"
#include "loader/include/firmware.h"
#include "loader/include/boot.h"
#include "loader/include/loader.h"


/*
 * SiFive UART
 */
// Virt @ 0x10000000, SiFive-U @ 0x10013000
#define UART_BASE_ADDR          (0x10000000)    //(0x10000000)
#define UART_DATA_ADDR          (UART_BASE_ADDR + 0x0)
#define UART_LINE_STAT_ADDR     (UART_BASE_ADDR + 0x28)

static void mmio_write8(unsigned long addr, unsigned char val)
{
    volatile unsigned char *ptr = (void *)addr;
    *ptr = val;
}

int arch_debug_putchar(int ch)
{
/*    u32 ready = 0;*/
/*    while (!ready) {*/
/*        ready = mmio_read8(UART_LINE_STAT_ADDR) & 0x20;*/
/*    }*/

    mmio_write8(UART_DATA_ADDR, ch & 0xff);
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

static void *setup_page()
{
    root_page = alloc_page();
    memzero(root_page, PAGE_SIZE);

    return root_page;
}

static void verify_cacheability(u64 paddr, u64 size)
{
    u64 dram_start = 0;
    u64 dram_range = get_memmap_range(&dram_start);

    panic_if(paddr < dram_start || paddr + size > dram_start + dram_range,
        "Trying to mark uncacheable memory as cacheable @ %llx, size: %llx",
        paddr, size);
}

static void map_page(void *page_table, void *vaddr, u64 paddr, int block,
    int cache, int exec, int write)
{
    struct page_frame *page = page_table;
    struct page_table_entry *entry = NULL;

    for (int level = 0; level <= block; level++) {
        int idx = (int)GET_PTE_INDEX((ulong)vaddr, level);
        entry = &page->entries[idx];

        if (level != block) {
            if (!entry->valid) {
                page = alloc_page();
                memzero(page, sizeof(struct page_frame));

                entry->valid = 1;
                entry->pfn = ADDR_TO_PFN((ulong)page);
            } else {
                page = (void *)PFN_TO_ADDR((ulong)entry->pfn);
            }
        }
    }

    if (entry->valid) {
        panic_if((ulong)entry->pfn != (ulong)ADDR_TO_PFN(paddr) ||
            entry->read != !1 ||
            entry->write != write ||
            entry->exec != exec,
            "Trying to change an existing mapping @ %p"
            ", existing PFN @ %lx, new PFN @ %lx!",
            vaddr, (ulong)entry->pfn, (ulong)ADDR_TO_PFN(paddr));
    } else {
        entry->valid = 1;
        entry->pfn = (ulong)ADDR_TO_PFN(paddr);

        entry->read = 1;
        entry->write = write;
        entry->exec = exec;
    }
}

static int map_range(void *page_table, void *vaddr, void *ppaddr, ulong size,
    int cache, int exec, int write)
{
    lprintf("Map range @ %p -> %p, size: %lx, cache: %d, exec: %d, write: %d\n",
        vaddr, ppaddr, size, cache, exec, write);

    u64 paddr = (ulong)ppaddr;
    if (cache) {
        verify_cacheability(paddr, size);
    }

    ulong vaddr_start = ALIGN_DOWN((ulong)vaddr, PAGE_SIZE);
    ulong vaddr_end = ALIGN_UP((ulong)vaddr + size, PAGE_SIZE);
    u64 paddr_start = ALIGN_DOWN(paddr, PAGE_SIZE);

    int mapped_pages = 0;
    if (vaddr_start >> 63) {
        page_table += PAGE_SIZE;
    }

    u64 cur_paddr = paddr_start;
    for (ulong cur_vaddr = vaddr_start; cur_vaddr < vaddr_end; ) {
        // L0 block
        if (ALIGNED(cur_vaddr, L0BLOCK_SIZE) &&
            ALIGNED(cur_paddr, L0BLOCK_SIZE) &&
            vaddr_end - cur_vaddr >= L0BLOCK_SIZE
        ) {
            map_page(page_table, (void *)cur_vaddr, cur_paddr, 0,
                cache, exec, write);

            mapped_pages += L0BLOCK_PAGE_COUNT;
            cur_vaddr += L0BLOCK_SIZE;
            cur_paddr += L0BLOCK_SIZE;
        }

#if (defined(ARCH_RISCV64))
        // L1 block
        else if (ALIGNED(cur_vaddr, L1BLOCK_SIZE) &&
            ALIGNED(cur_paddr, L1BLOCK_SIZE) &&
            vaddr_end - cur_vaddr >= L1BLOCK_SIZE
        ) {
            map_page(page_table, (void *)cur_vaddr, cur_paddr, 1,
                cache, exec, write);

            mapped_pages += L1BLOCK_PAGE_COUNT;
            cur_vaddr += L1BLOCK_SIZE;
            cur_paddr += L1BLOCK_SIZE;
        }

#   if (VADDR_BITS == 48)
        // L2 block
        else if (ALIGNED(cur_vaddr, L2BLOCK_SIZE) &&
            ALIGNED(cur_paddr, L2BLOCK_SIZE) &&
            vaddr_end - cur_vaddr >= L2BLOCK_SIZE
        ) {
            map_page(page_table, (void *)cur_vaddr, cur_paddr, 2,
                cache, exec, write);

            mapped_pages += L2BLOCK_PAGE_COUNT;
            cur_vaddr += L2BLOCK_SIZE;
            cur_paddr += L2BLOCK_SIZE;
        }
#   endif
#endif

        // 4KB page
        else {
            map_page(page_table, (void *)cur_vaddr, cur_paddr,
                PAGE_LEVELS - 1, cache, exec, write);

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
    // FIXME: also need to map device
    map_range(root_page, (void *)UART_BASE_ADDR, (void *)UART_BASE_ADDR, 0x100, 0, 0, 1);

    struct supervisor_addr_trans_prot_reg stap;
    stap.value = 0;

    stap.pfn = ADDR_TO_PFN((ulong)root_page);
    stap.asid = 0;
#if (defined(ARCH_RISCV64) && VADDR_BITS == 39)
    stap.mode = STAP_MODE_SV39;
#elif (defined(ARCH_RISCV64) && VADDR_BITS == 48)
    stap.mode = STAP_MODE_SV48;
#elif (defined(ARCH_RISCV32))
    stap.mode = STAP_MODE_SV32;
#endif

    write_satp(stap.value);

//     arch_debug_putchar('g');
//     arch_debug_putchar('o');
//     arch_debug_putchar('o');
//     arch_debug_putchar('d');
//     arch_debug_putchar('\n');
//     while (1);
}

typedef void (*hal_start_t)(struct loader_args *largs);

static void call_hal(struct loader_args *largs)
{
    hal_start_t hal = largs->hal_entry;
    hal(largs);
}

static void jump_to_hal()
{
    struct loader_args *largs = get_loader_args();
    lprintf("Jump to HAL @ %p\n", largs->hal_entry);

    enable_mmu(largs->page_table);
    lprintf("Page root @ %p\n", largs->page_table);

    call_hal(largs);
}


/*
 * Init arch
 */
static unsigned char dup_fdt[4096];

static void duplicate_fdt()
{
    // Duplicate FDT to writable memory region
    struct firmware_args *fw_args = get_fw_args();
    if (fw_args->type == FW_FDT) {
        copy_fdt(dup_fdt, fw_args->fdt.fdt, sizeof(dup_fdt));
        fw_args->fdt.fdt = dup_fdt;
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
    struct machine_status_reg mstatus;
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

static void init_arch()
{
    duplicate_fdt();
    setup_pmp();
    enter_supervisor_mode();
}


/*
 * The RISC-V entry point
 */
void loader_entry(ulong cfg_type, void *cfg)
{
    lprintf("cfg_type: %d, cfg @ %p, magic: %x\n", cfg_type, cfg, *(u32 *)cfg);

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
    if (cfg_type == 1) {
        fw_args.type = FW_FDT;
        fw_args.fdt.fdt = cfg;
    } else {
        fw_args.type = FW_NONE;
    }

    // Prepare arch info
//     funcs.reserved_stack_size = 0x8000;
    funcs.page_size = PAGE_SIZE;
//     funcs.num_reserved_got_entries = ELF_GOT_NUM_RESERVED_ENTRIES;

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
