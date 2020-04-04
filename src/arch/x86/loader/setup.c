#include "common/include/inttypes.h"
#include "common/include/io.h"
#include "common/include/mem.h"
// #include "common/include/abi.h"
#include "common/include/msr.h"
#include "loader/include/header.h"
#include "loader/include/lib.h"
#include "loader/include/firmware.h"
#include "loader/include/boot.h"
#include "loader/include/loader.h"


/*
 * Raspi2 UART
 */
#define SERIAL_PORT (0x3f8)
#define SERIAL_DR   (SERIAL_PORT)
#define SERIAL_FR   (SERIAL_PORT + 0x5)

int arch_debug_putchar(int ch)
{
    // Wait until the UART has an empty space in the FIFO
    u32 empty = 0;
    while (empty) {
        empty = !(ioport_read8(SERIAL_FR) & 0x20);
    }

    // Write the character to the FIFO for transmission
    ioport_write8(SERIAL_DR, ch & 0xff);

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

#if (defined(ARCH_AMD64))

static void map_page(void *page_table, void *vaddr, void *paddr,
    int cache, int exec, int write)
{
    struct page_frame *page = page_table;
    struct page_table_entry *entry = NULL;

    for (int level = 0; level < PAGE_LEVELS; level++) {
        int idx = (int)GET_PAGE_TABLE_ENTRY_INDEX((ulong)vaddr, level);
        entry = &page->entries[idx];

        if (level != PAGE_LEVELS - 1) {
            if (!entry->present) {
                page = alloc_page();
                memzero(page, sizeof(struct page_frame));

                entry->present = 1;
                entry->pfn = ADDR_TO_PFN((ulong)page);
                entry->write_allow = 1;
            } else {
                page = (void *)PFN_TO_ADDR((ulong)entry->pfn);
            }
        }
    }

    if (entry->present) {
        panic_if((ulong)entry->pfn != ADDR_TO_PFN((ulong)paddr) ||
            entry->write_allow != write ||
            entry->cache_disabled != !cache ||
            //entry->no_exec != !exec ||
            entry->user_allow != 0,
            "Trying to change an existing mapping from PFN %lx to %lx!",
            (ulong)entry->pfn, ADDR_TO_PFN((ulong)paddr));
    } else {
        entry->present = 1;
        entry->pfn = ADDR_TO_PFN((ulong)paddr);
        entry->write_allow = write;
        entry->cache_disabled = !cache;
        //entry->no_exec = !exec;
    }
}

#elif (defined(ARCH_IA32))

static void map_page(void *page_table, void *vaddr, void *paddr,
    int cache, int exec, int write)
{
    // First level
    struct page_frame *l1table = page_table;
    int l1idx = GET_PDE_INDEX((ulong)vaddr);
    struct page_table_entry *l1entry = &l1table->entries[l1idx];

    // Second level
    struct page_frame *l2table = NULL;
    if (!l1entry->present) {
        l2table = alloc_page();
        memzero(l2table, PAGE_SIZE);

        l1entry->present = 1;
        l1entry->pfn = ADDR_TO_PFN((ulong)l2table);
        l1entry->write_allow = 1;
        l1entry->user_allow = 1;
    } else {
        l2table = (void *)PFN_TO_ADDR((ulong)l1entry->pfn);
    }

    // The mapping
    int l2idx = GET_PTE_INDEX((ulong)vaddr);
    struct page_table_entry *l2entry = &l2table->entries[l2idx];

    if (l2entry->present) {
        panic_if(
            (ulong)l2entry->pfn != ADDR_TO_PFN((ulong)paddr) ||
            l2entry->write_allow != write ||
            l2entry->cache_disabled != !cache ||
            //l2entry->no_exec != !exec ||
            l2entry->user_allow != 0,
            "Trying to change an existing mapping from PFN %lx to %lx!",
            (ulong)l2entry->pfn, ADDR_TO_PFN((ulong)paddr));
    }

    else {
        l2entry->present = 1;
        l2entry->pfn = ADDR_TO_PFN((ulong)paddr);
        l2entry->write_allow = write;
        l2entry->user_allow = 1;
        l2entry->cache_disabled = !cache;
        //l2entry->no_exec = !exec;
    }
}

#endif

static int map_range(void *page_table, void *vaddr, void *paddr, ulong size,
    int cache, int exec, int write)
{
    kprintf("To map range @ %p -> %p, size: %lx\n", vaddr, paddr, size);

    ulong vaddr_start = ALIGN_DOWN((ulong)vaddr, PAGE_SIZE);
    ulong paddr_start = ALIGN_DOWN((ulong)paddr, PAGE_SIZE);
    ulong vaddr_end = ALIGN_UP((ulong)vaddr + size, PAGE_SIZE);

    int mapped_pages = 0;

    for (ulong cur_vaddr = vaddr_start, cur_paddr = paddr_start;
        cur_vaddr < vaddr_end;
    ) {
        // 4KB page
        map_page(page_table, (void *)cur_vaddr, (void *)cur_paddr,
            cache, exec, write);

        mapped_pages++;
        cur_vaddr += PAGE_SIZE;
        cur_paddr += PAGE_SIZE;
    }

    return mapped_pages;
}


/*
 * Jump to HAL
 */
static void enable_mmu(void *root_page)
{
    struct ctrl_reg3 cr3;
    cr3.value = 0;
    cr3.pfn = ADDR_TO_PFN((ulong)root_page);
    write_cr3(cr3.value);
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
    kprintf("Jump to HAL @ %p\n", largs->hal_entry);

    enable_mmu(largs->page_table);
    kprintf("Page root @ %p\n", largs->page_table);

    call_hal(largs);
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
void loader_entry(ulong magic, void *mbi)
{
    kprintf("Hello!\n");
    kprintf("Hello, magic: %lx, mbi @ %p!\n", magic, mbi);
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

    // Prepare arg
    if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
        fw_args.type = FW_MULTIBOOT;
        fw_args.multiboot.multiboot = mbi;
    } else {
        fw_args.type = FW_NONE;
    }

    // Prepare arch info
    funcs.reserved_stack_size = 0x8000;
    funcs.page_size = PAGE_SIZE;
    //funcs.num_reserved_got_entries = ELF_GOT_NUM_RESERVED_ENTRIES;

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
