#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "common/include/page.h"
#include "common/include/abi.h"
#include "common/include/msr.h"
#include "loader/include/lib.h"
#include "loader/include/mem.h"
#include "loader/include/boot.h"
#include "loader/include/loader.h"
#include "loader/include/firmware.h"
#include "loader/include/kprintf.h"
#include "loader/include/header.h"
#include "loader/include/devtree.h"


/*
 * x86 PC UART
 */
#define SERIAL_PORT (0x3f8)
#define SERIAL_DR   (SERIAL_PORT)
#define SERIAL_FR   (SERIAL_PORT + 0x5)

static inline u8 ioport_read8(u16 port)
{
    u8 data;

    __asm__ __volatile__ (
        "inb %[port], %[data];"
        : [data] "=a" (data)
        : [port] "Nd" (port)
        : "memory"
    );

    return data;
}

static inline void ioport_write8(u16 port, u8 data)
{
    __asm__ __volatile__ (
        "outb %[data], %[port];"
        :
        : [data] "a" (data), [port] "Nd" (port)
        : "memory"
    );
}

static int x86_pc_putchar(int ch)
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
 * Paging
 */
static void *alloc_page()
{
    paddr_t paddr = memmap_alloc_phys(PAGE_SIZE, PAGE_SIZE);
    return cast_paddr_to_ptr(paddr);
}

static void *setup_page()
{
    struct page_frame *root_page = alloc_page();
    memzero(root_page, sizeof(struct page_frame));

    return root_page;
}

static int map_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size,
                     int cache, int exec, int write)
{
    return generic_map_range(page_table, vaddr, paddr, size, cache, exec, write, 1, 0);
}


/*
 * Jump to HAL
 */
static void enable_mmu(void *root_page)
{
    struct ctrl_reg3 cr3;
    cr3.value = 0;
    cr3.pfn = vaddr_to_vpfn((ulong)root_page);
    write_cr3(cr3.value);
}

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

    enable_mmu(largs->page_table);
    kprintf("Page root @ %p\n", largs->page_table);

    call_hal(largs, 0);
}

static void jump_to_hal_mp()
{
    struct loader_args *largs = get_loader_args();
    kprintf("MP Jump to HAL @ %p\n", largs->hal_entry);

    enable_mmu(largs->page_table);
    kprintf("Page root @ %p\n", largs->page_table);

    call_hal(largs, 1);
}


/*
 * Finalize
 */
static void final_memmap()
{
    // FIXME: simply assume all the memory is direct mapped
    // may not be true on some machines, e.g. PAE and more t han 4GB memory
    u64 start = 0;
    u64 size = get_memmap_range(&start);
    tag_memmap_region(start, size, MEMMAP_TAG_DIRECT_MAPPED);
}

static void final_arch()
{
}


/*
 * Init arch
 */
static void init_arch()
{
}

static void init_arch_mp()
{
}


/*
 * Init libk
 */
static void init_libk()
{
    init_libk_putchar(x86_pc_putchar);
    init_libk_page_table(memmap_palloc, NULL, NULL);
}


/*
 * The ARM entry point
 */
void loader_entry(ulong magic, void *mbi)
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
    if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
        fw_args.fw_name = "multiboot";
        fw_args.fw_params = mbi;
    } else {
        fw_args.fw_name = "none";
    }

    // Stack limit
    extern ulong _stack_limit, _stack_limit_mp;
    funcs.stack_limit = (ulong)&_stack_limit;
    funcs.stack_limit_mp = (ulong)&_stack_limit_mp;

    // MP entry
    extern ulong _start_mp;
    funcs.mp_entry = (ulong)&_start_mp;

    // Prepare funcs
    funcs.init_libk = init_libk;
    funcs.init_arch = init_arch;
    funcs.setup_page = setup_page;
    funcs.map_range = map_range;
    funcs.final_memmap = final_memmap;
    funcs.final_arch = final_arch;
    funcs.jump_to_hal = jump_to_hal;

    // MP funcs
    funcs.init_arch_mp = init_arch_mp;
    funcs.jump_to_hal_mp = jump_to_hal_mp;

    // Go to loader!
    loader(&fw_args, &funcs);
    unreachable();
}

void loader_entry_mp()
{
    kprintf("Loader MP!\n");

    // Go to loader!
    loader_mp();
    unreachable();
}
