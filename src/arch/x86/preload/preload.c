#include "common/include/inttypes.h"
#include "common/include/msr.h"
#include "common/include/mem.h"
#include "common/include/gdt.h"
#include "common/include/page.h"


/*
 * This symbol is declared as weak so that the actual coreimg can override this
 * The bin2c tool is used to convert the coreimg binary to C, and later compiled
 * to the loader ELF
 */
unsigned char __attribute__((weak, aligned(8))) embedded_bootimg[] = {
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
};
unsigned long __attribute__((weak)) embedded_bootimg_bytes = 0;


/*
 * Shared by both IA32 and AMD64
 */
extern int __bss_start;
extern int __bss_end;

static void init_bss()
{
    int *cur;

    for (cur = &__bss_start; cur < &__bss_end; cur++) {
        *cur = 0;
    }
}

static void move_bootimg()
{
    if (!embedded_bootimg_bytes) {
        while (1);
    }

    else if ((ulong)embedded_bootimg > LOADER_BASE) {
        u8 *src = embedded_bootimg;
        u8 *dest = (void *)(ulong)LOADER_BASE;
        for (ulong i = 0; i < embedded_bootimg_bytes; i++) {
            *dest++ = *src++;
        }
    }

    else if ((ulong)embedded_bootimg < LOADER_BASE) {
        u8 *src = (void *)((ulong)embedded_bootimg + embedded_bootimg_bytes);
        u8 *dest = (void *)((ulong)LOADER_BASE + embedded_bootimg_bytes);
        for (ulong i = 0; i <= embedded_bootimg_bytes; i++) {
            *dest-- = *src--;
        }
    }
}

static void enable_mmu(struct page_frame *root_page)
{
    // Enable Page Size Extension
    struct ctrl_reg4 cr4;
    read_cr4_32(cr4.value);
    cr4.page_size_ext = 1;
    write_cr4_32(cr4.value);

    // Load page table
    struct ctrl_reg3 cr3;
    cr3.value = 0;
    cr3.pfn = (ulong)root_page >> PAGE_BITS;
    write_cr3_32(cr3.value);

    // Enable paging
    struct ctrl_reg0 cr0;
    read_cr0_32(cr0.value);
    cr0.paging = 1;
    write_cr0_32(cr0.value);
}


/*
 * AMD64-specific
 */
#if (defined(ARCH_AMD64))

struct global_desc_table {
    struct global_desc_table_entry dummy;
    struct global_desc_table_entry code;
    struct global_desc_table_entry data;
} natural_struct;

static struct global_desc_table gdt64 aligned_var(16);
static struct global_desc_table_ptr gdt64_ptr aligned_var(16);

static struct page_frame pages[3] aligned_var(PAGE_SIZE);

static void enter_compatibility_mode()
{
    // Enable PAE
    struct ctrl_reg4 cr4;
    read_cr4_32(cr4.value);
    cr4.phys_addr_ext = 1;
    write_cr4_32(cr4.value);

    // Enable long mode
    struct ext_feature_enable_reg efer;
    read_efer(efer.value);
    efer.long_mode = 1;
    write_efer(efer.value);
}

static void setup_page()
{
    for (int p = 0; p < 3; p++) {
        for (int i = 0; i < PAGE_TABLE_ENTRY_COUNT; i++) {
            pages[p].entries[i].value = 0;
        }
    }

    struct page_table_entry *entry = NULL;

    entry = &pages[0].entries[0];
    entry->present = 1;
    entry->pfn = (ulong)&pages[1] >> PAGE_BITS;
    entry->write_allow = 1;
    entry->cache_disabled = 1;

    entry = &pages[1].entries[0];
    entry->present = 1;
    entry->pfn = (ulong)&pages[2] >> PAGE_BITS;
    entry->write_allow = 1;
    entry->cache_disabled = 1;

    u64 cur_paddr = 0;
    entry = &pages[2].entries[0];
    for (int i = 0; i < PAGE_TABLE_ENTRY_COUNT; i++,
        cur_paddr += 0x200000ull, entry++
    ) {
        entry->present = 1;
        entry->pfn = cur_paddr >> PAGE_BITS;
        entry->write_allow = 1;
        entry->cache_disabled = 1;
        entry->large_page = 1;
    }
}

static void enter_long_mode_and_call_boot(ulong ptr, ulong magic, ulong target)
{
    gdt64.dummy.value = 0;
    gdt64.dummy.granularity = 1;
    gdt64.dummy.limit_low = 0xffff;
    gdt64.dummy.limit_high = 0xf;

    gdt64.code.value = 0;
    gdt64.code.read_write = 1;
    gdt64.code.exec = 1;
    gdt64.code.type = 1;
    gdt64.code.present = 1;
    gdt64.code.limit_low = 0xffff;
    gdt64.code.limit_high = 0xf;
    gdt64.code.amd64_seg = 1;
    gdt64.code.granularity = 1;

    gdt64.data.value = 0;
    gdt64.data.read_write = 1;
    gdt64.data.type = 1;
    gdt64.data.present = 1;
    gdt64.data.limit_low = 0xffff;
    gdt64.data.limit_high = 0xf;
    gdt64.data.granularity = 1;

    gdt64_ptr.base = (u64)(ulong)&gdt64;
    gdt64_ptr.limit = sizeof(gdt64) - 1;

    //kprintf("GDT entry size: %d\n", sizeof(struct global_desc_table_entry));

    __asm__ __volatile__ (
        "lgdtl %[gdt_ptr];"
        "jmpl $0x8, $_call_hal64;"
        "jmp .;"
        :
        : [gdt_ptr] "m" (*&gdt64_ptr),
          "D" (ptr), "S" (magic), "d" (target)
    );
}

static void preload(ulong ptr, ulong magic, ulong target)
{
    enter_compatibility_mode();
    enable_mmu(&pages[0]);
    enter_long_mode_and_call_boot(ptr, magic, target);
}

/*
 * IA32-specific
 */
#elif (defined(ARCH_IA32))

static struct page_frame page aligned_var(PAGE_SIZE);

static void setup_page()
{
    for (int i = 0; i < PAGE_TABLE_ENTRY_COUNT; i++) {
        page.entries[i].value = 0;
    }

    u64 cur_paddr = 0;
    struct page_table_entry *entry = &page.entries[0];

    for (int i = 0; i < PAGE_TABLE_ENTRY_COUNT; i++,
        cur_paddr += 0x400000ull, entry++
    ) {
        entry->present = 1;
        entry->pfn = cur_paddr >> PAGE_BITS;
        entry->write_allow = 1;
        entry->cache_disabled = 1;
        entry->large_page = 1;
    }
}

static void call_boot(ulong ptr, ulong magic, ulong target)
{
    __asm__ __volatile__ (
        "jmp *%%ebx;"
        :
        : "a" (ptr), "d" (magic), "b" (target)
    );
}

static void preload(ulong ptr, ulong magic, ulong target)
{
    enable_mmu(&page);
    call_boot(ptr, magic, target);
}

#endif


/*
 * Entry
 */
void preload_entry(ulong ptr, ulong magic)
{
    init_bss();
    move_bootimg();
    setup_page();

    preload(ptr, magic, LOADER_BASE);
    while (1);
}
