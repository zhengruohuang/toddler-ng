#include "common/include/inttypes.h"
#include "loader/include/lib.h"
#include "loader/include/firmware.h"
#include "loader/include/boot.h"
#include "loader/include/loader.h"


/*
 * Dummy UART
 */
int arch_debug_putchar(int ch)
{
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
//static struct page_frame *root_page = NULL;

static void *alloc_page()
{
    return NULL;

//     void *paddr = memmap_alloc_phys(PAGE_SIZE, PAGE_SIZE);
//     return (void *)((ulong)paddr);
}

static void *setup_page()
{
    return NULL;

//     root_page = alloc_page();
//     memzero(root_page, PAGE_SIZE);
//
//     return root_page;
}

// static void map_page(void *page_table, void *vaddr, u64 paddr, int block,
//     int cache, int exec, int write)
// {
//
// }

static int map_range(void *page_table, void *vaddr, void *ppaddr, ulong size,
    int cache, int exec, int write)
{
    return 0;

//     u64 paddr = (ulong)ppaddr;
//
//     ulong vaddr_start = ALIGN_DOWN((ulong)vaddr, PAGE_SIZE);
//     ulong vaddr_end = ALIGN_UP((ulong)vaddr + size, PAGE_SIZE);
//     u64 paddr_start = ALIGN_DOWN(paddr, PAGE_SIZE);
//
//     int mapped_pages = 0;
//     if (vaddr_start >> 63) {
//         page_table += PAGE_SIZE;
//     }
//
//     u64 cur_paddr = paddr_start;
//     for (ulong cur_vaddr = vaddr_start; cur_vaddr < vaddr_end; ) {
//         // L0 block
//         if (ALIGNED(cur_vaddr, L0BLOCK_SIZE) &&
//             ALIGNED(cur_paddr, L0BLOCK_SIZE) &&
//             vaddr_end - cur_vaddr >= L0BLOCK_SIZE
//         ) {
//             map_page(page_table, (void *)cur_vaddr, cur_paddr, 0,
//                 cache, exec, write);
//
//             mapped_pages += L0BLOCK_PAGE_COUNT;
//             cur_vaddr += L0BLOCK_SIZE;
//             cur_paddr += L0BLOCK_SIZE;
//         }
//
//         // 4KB page
//         else {
//             map_page(page_table, (void *)cur_vaddr, cur_paddr,
//                 PAGE_LEVELS - 1, cache, exec, write);
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

    call_hal(largs);
}


/*
 * Init arch
 */
static void init_arch()
{
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

    // Prepare arch info
//     funcs.reserved_stack_size = 0x8000;
//     funcs.page_size = PAGE_SIZE;
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

void _start()
{
    loader_entry();
}
