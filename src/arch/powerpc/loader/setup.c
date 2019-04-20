#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "loader/include/lib.h"
#include "loader/include/loader.h"
#include "loader/include/firmware.h"
#include "loader/include/boot.h"
#include "loader/include/arch.h"


/*
 * Mac EESC
 */
#define EESC_BASE       (0x80013020ul)
#define EESC_CTRL       (EESC_BASE)
#define EESC_DATA       (EESC_BASE + 0x10ul)

#define ESCC_REG_CTRL    0
#define ESCC_BES_TXE     0x4

int arch_debug_putchar(int ch)
{
    volatile unsigned char *ctrl = (void *)EESC_CTRL;
    volatile unsigned char *data = (void *)EESC_DATA;

    unsigned char empty = 0;
    while (!empty) {
        *ctrl = ESCC_REG_CTRL;
        empty = *ctrl & ESCC_BES_TXE;
    }

    *data = ch;

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
 * PHT
 */
static struct arch_loader_args arch_loader_args;

static struct pht_group *pht = NULL;
static void *pht_phys = NULL;

static struct pht_attri_group *pht_attri = NULL;
static void *pht_attri_phys = NULL;

static ulong pht_size = 0;
static ulong pht_hash_mask = 0;
static ulong pht_group_count = 0;

static void init_pht()
{
    // Find out PHT size
    u64 aligned_range = get_memmap_range(NULL);
    aligned_range = align_to_power2_next64(aligned_range);
    pht_size = aligned_range >> 7;
    pht_hash_mask = (pht_size >> 16) - 1;
    pht_group_count = pht_size / sizeof(struct pht_group);

    lprintf("Aligned memory range: %llx, PHT size: %lx\n", aligned_range, pht_size);

    // Allocate and map PHT
    pht_phys = memmap_alloc_phys(pht_size, pht_size);
    pht = firmware_alloc_and_map_acc_win(pht_phys, pht_size, 8);

    // Allocate and map PHT attri
    ulong pht_attri_size = pht_group_count * sizeof(struct pht_attri_group);
    pht_attri_phys = memmap_alloc_phys(pht_attri_size, pht_attri_size);
    pht_attri = firmware_alloc_and_map_acc_win(pht_attri_phys, pht_attri_size, 8);

    // Init PHT and PHT attri
    for (int i = 0; i < pht_group_count; i++) {
        for (int j = 0; j < 8; j++) {
            pht[i].entries[j].word0 = pht[i].entries[j].word1 = 0;
            pht_attri[i].entries[j].value = 0;
        }
    }
}

static int pht_index(u32 vaddr, int secondary)
{
    // Calculate the hash
    //   Note for HAL and kernel VSID is just the higher 4 bits of EA
    u32 val_vsid = vaddr >> 28;
    u32 val_pfn = ADDR_TO_PFN(vaddr) & 0xffff;
    u32 hash = val_vsid ^  val_pfn;

    // Take care of secondary hash
    if (secondary) {
        hash = ~hash + 1;
    }

    // Calculate index
    ulong left = (hash >> 10) & pht_hash_mask & 0x1fff;    //(hash >> 10) & LOADER_PHT_MASK & 0x1ff;
    ulong right = hash & 0x3ff;
    int index = (int)((left << 10) | right);

    return index;
}

static struct pht_entry *find_free_pht_entry(u32 vaddr, int *group, int *offset)
{
    int i, idx, secondary;
    struct pht_entry *entry = NULL;

    for (secondary = 0; secondary <= 1; secondary++) {
        idx = pht_index(vaddr, secondary);

        for (i = 0; i < 8; i++) {
            entry = &pht[idx].entries[i];
            if (!entry->valid) {
                if (group) *group = idx;
                if (offset) *offset = i;

                entry->secondary = secondary;
                return entry;
            }
        }
    }

    panic("Unable to find a free PHT entry @ %p\n", vaddr);
    return NULL;
}

static void pht_fill(ulong vaddr, ulong paddr, ulong len, int io)
{
//     ofw_printf("\tTo fill PHT @ %p to %p\n", vstart, end);

    ulong end = vaddr + len;
    for (; vaddr < end; vaddr += PAGE_SIZE, paddr += PAGE_SIZE) {
        ulong phys_pfn = ADDR_TO_PFN(paddr);

        int group = 0, offset = 0;
        struct pht_entry *entry = find_free_pht_entry(vaddr, &group, &offset);
        panic_if(!entry, "Unable to find a free PHT entry @ %p\n", vaddr);

        // Set up entry
        entry->valid = 1;
        entry->page_idx = (ADDR_TO_PFN(vaddr) >> 10) & 0x3f;
        entry->vsid = vaddr >> 28;
        entry->pfn = phys_pfn;
        entry->protect = 0x2;   // FIXME, 0x2 = R/W in both user and kernel
        if (io) {
            entry->no_cache = 1;
            entry->guarded = 1;
        } else {
            entry->coherent = 1;
        }

        // Set up Toddler special attributes
        pht_attri[group].entries[offset].persist = 1;
    }
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

    init_pht();

    return root_page;
}

static void map_page(void *page_table, void *vaddr, void *paddr, int block,
    int cache, int exec, int write)
{
    //lprintf("To map %p -> %p, block: %d\n", vaddr, paddr, block);

    struct page_frame *table = page_table;
    int l0idx = GET_L0PTE_INDEX((ulong)vaddr);
    struct page_table_entry *entry = &table->entries[l0idx];

    // Walk to the next level
    if (!block) {
        struct page_frame *l1table = NULL;
        if (!entry->present) {
            l1table = alloc_page();
            memzero(l1table, PAGE_SIZE);

            entry->present = 1;
            entry->next_level = 1;
            entry->pfn = ADDR_TO_PFN((ulong)l1table);

            //lprintf("Next level PFN2 @ %x\n", entry->pfn);
        } else {
            //if (!entry->next_level) {
            //    lprintf("Next level PFN @ %x\n", entry->pfn);
            //}
            panic_if(!entry->next_level, "Must have next level!");
            l1table = (void *)PFN_TO_ADDR((ulong)entry->pfn);
        }

        int l1idx = GET_L1PTE_INDEX((ulong)vaddr);
        entry = &l1table->entries[l1idx];
    }

    // Last-level
    if (entry->present) {
        panic_if(
            (ulong)entry->pfn != ADDR_TO_PFN((ulong)paddr) ||
            entry->write_allow != exec ||
            entry->exec_allow != write ||
            entry->cache_allow != cache,
            "Trying to change an existing mapping from BFN %lx to %lx!",
            (ulong)entry->pfn, ADDR_TO_PFN((ulong)paddr));
    }

    else {
        entry->present = 1;
        entry->pfn = ADDR_TO_PFN((ulong)paddr);
        entry->read_allow = 1;
        entry->write_allow = write;
        entry->exec_allow = exec;
        entry->cache_allow = cache;
        entry->kernel = 1;
    }
}

static int map_range(void *page_table, void *vaddr, void *paddr, ulong size,
    int cache, int exec, int write)
{
    ulong vaddr_start = ALIGN_DOWN((ulong)vaddr, PAGE_SIZE);
    ulong paddr_start = ALIGN_DOWN((ulong)paddr, PAGE_SIZE);
    ulong vaddr_end = ALIGN_UP((ulong)vaddr + size, PAGE_SIZE);

    int mapped_pages = 0;

    for (ulong cur_vaddr = vaddr_start, cur_paddr = paddr_start;
        cur_vaddr < vaddr_end;
    ) {
        // 4MB block
        if (ALIGNED(cur_vaddr, L0BLOCK_SIZE) &&
            ALIGNED(cur_paddr, L0BLOCK_SIZE) &&
            vaddr_end - cur_vaddr >= L0BLOCK_SIZE
        ) {
            map_page(page_table, (void *)cur_vaddr, (void *)cur_paddr, 1,
                cache, exec, write);

            mapped_pages += L0BLOCK_PAGE_COUNT;
            cur_vaddr += L0BLOCK_SIZE;
            cur_paddr += L0BLOCK_SIZE;
        }

        // 4KB page
        else {
            map_page(page_table, (void *)cur_vaddr, (void *)cur_paddr, 0,
                cache, exec, write);

            mapped_pages++;
            cur_vaddr += PAGE_SIZE;
            cur_paddr += PAGE_SIZE;
        }
    }

    // Fill PHT
    pht_fill(vaddr_start, paddr_start, vaddr_end - vaddr_start, !cache);

    return mapped_pages;
}


/*
 * Jump to HAL
 */
typedef void (*call_hal_t)(struct loader_args *largs, void *hal_entry_vaddr);

extern int _stack_top;
extern void call_hal(struct loader_args *largs, void *hal_entry_vaddr);
extern void call_real_mode(struct loader_args *largs, void *call_hal_paddr,
                            void *real_mode_entry_paddr, void *stack_paddr);

static void real_mode_entry(struct loader_args *largs, void *call_hal_paddr)
{
    struct arch_loader_args *arch_largs = largs->arch_args;

    // Fill segment registers
    for (int i = 0; i < 16; i++) {
        struct seg_reg sr;

        sr.value = 0;
        sr.key_kernel = 1;
        sr.vsid = i;

        __asm__ __volatile__
        (
            "mtsrin %[val], %[idx];"
            :
            : [val] "r" (sr.value), [idx] "r" (i << 28)
        );
    }

    // Invalidate BAT registers
    __asm__ __volatile__
    (
        "mtspr 528, %[zero]; mtspr 529, %[zero];"
        "mtspr 530, %[zero]; mtspr 531, %[zero];"
        "mtspr 532, %[zero]; mtspr 533, %[zero];"
        "mtspr 534, %[zero]; mtspr 535, %[zero];"

        "mtspr 536, %[zero]; mtspr 537, %[zero];"
        "mtspr 538, %[zero]; mtspr 539, %[zero];"
        "mtspr 540, %[zero]; mtspr 541, %[zero];"
        "mtspr 542, %[zero]; mtspr 543, %[zero];"
        :
        : [zero] "r" (0)
    );

    // Fill in PHT
    __asm__ __volatile__
    (
        "mtsdr1 %[sdr1];"
        :
        : [sdr1] "r" ((ulong)arch_largs->pht | arch_largs->pht_mask)
    );

    for (int i = 0; i < arch_largs->pht_size / sizeof(struct pht_group); i++) {
        ulong tmp = (ulong)arch_largs->pht;

        for (int j = 0; j < 8; j++) {
            __asm__ __volatile__
            (
                "dcbst 0, %[addr];"
                "sync;"
                "icbi 0, %[addr];"
                "sync;"
                "isync;"
                :
                : [addr] "r" (tmp)
            );

            tmp += sizeof(struct pht_entry);
        }
    }

    // Flush TLB
    __asm__ __volatile__
    (
        "sync;"
    );

    {
        ulong tmp = 0;
        for (int i = 0; i < 64; i++) {
            __asm__ __volatile__
            (
                "tlbie %[addr];"
                :
                : [addr] "r" (tmp)
            );

            tmp += 0x1000;
        }
    }

    __asm__ __volatile__
    (
        "eieio;"
        "tlbsync;"
        "sync;"
    );

    // Go to HAL
    call_hal_t call_hal_phys = call_hal_paddr;
    call_hal_phys(largs, largs->hal_entry);
}

static void jump_to_hal()
{
    struct loader_args *largs = get_loader_args();
    lprintf("Jump to HAL @ %p\n", largs->hal_entry);

    // Set up arch loader args
    arch_loader_args.pht = firmware_translate_virt_to_phys(pht);
    arch_loader_args.pht_size = pht_size;
    arch_loader_args.pht_mask = pht_hash_mask;
    largs->arch_args = &arch_loader_args;

    // Call real mode
    void *stack_paddr = firmware_translate_virt_to_phys(&_stack_top);
    void *largs_paddr = firmware_translate_virt_to_phys(largs);
    void *real_mode_entry_paddr = firmware_translate_virt_to_phys(real_mode_entry);
    void *call_hal_paddr = firmware_translate_virt_to_phys(call_hal);
    lprintf("largs_paddr @ %p, real_mode_entry_paddr @ %p, call_hal_paddr @ %p\n",
            largs_paddr, real_mode_entry_paddr, call_hal_paddr);

    call_real_mode(largs_paddr, real_mode_entry_paddr, call_hal_paddr, stack_paddr);
}


/*
 * Init arch
 */
static void init_arch()
{

}


/*
 * The PowerPC entry point
 */
void loader_entry(void *initrd_addr, ulong initrd_size, void *ofw_entry)
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
    if (initrd_addr && is_fdt_header(initrd_addr)) {
    } else {
        fw_args.type = FW_OFW;
        fw_args.ofw.ofw = ofw_entry;
        fw_args.ofw.initrd_start = initrd_addr;
        fw_args.ofw.initrd_size = initrd_size;
    }

    // Prepare arch info
    // Note here we don't explicit reserve stack because it is directly
    // declared in start.S
    funcs.reserved_stack_size = 0;
    funcs.page_size = PAGE_SIZE;
//     funcs.num_reserved_got_entries = ELF_GOT_NUM_RESERVED_ENTRIES;
    funcs.phys_mem_range_min = PHYS_MEM_RANGE_MIN;
    funcs.phys_mem_range_max = PHYS_MEM_RANGE_MAX;

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
