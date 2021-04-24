#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/page.h"
#include "hal/include/setup.h"
#include "hal/include/mem.h"
#include "hal/include/lib.h"


static int mp_seq_setup = 0;
static int apic_setup = 0;
static get_apic_id_t get_apic_id = NULL;


ulong get_cur_mp_id()
{
    panic_if(!apic_setup, "APIC hasn't been set up!\n");
    return get_apic_id ? get_apic_id() : 0;
}

int get_cur_mp_seq()
{
    panic_if(!mp_seq_setup, "MP seq hasn't been set up!\n");
    u32 mp_seq = 0;

    __asm__ __volatile__ (
        "movl %%gs:(%%esi), %%edi;"
        : "=D" (mp_seq)
        : "S" (0)
        : "memory", "cc"
    );

    return mp_seq;
}

void setup_mp_id(get_apic_id_t f)
{
    apic_setup = 1;
    get_apic_id = f;
}

void setup_mp_seq()
{
    mp_seq_setup = 1;
}


/*
 * Init
 */
extern void _mp_boot_start();
extern void _mp_boot_end();

static ulong _translate(void *ptr)
{
    ulong vaddr = (ulong)ptr;
    paddr_t paddr = hal_translate(vaddr);
    return cast_paddr_to_vaddr(paddr);
}

#if (defined(ARCH_AMD64))

static struct page_frame pages[3] aligned_var(PAGE_SIZE);
static struct page_frame *page_table_root = &pages[0];

static void setup_boot_page_table()
{
    for (int p = 0; p < 3; p++) {
        for (int i = 0; i < PAGE_TABLE_ENTRY_COUNT; i++) {
            pages[p].entries[i].value = 0;
        }
    }

    struct page_table_entry *entry = NULL;

    entry = &pages[0].entries[0];
    entry->present = 1;
    entry->pfn = _translate(&pages[1]) >> PAGE_BITS;
    entry->write_allow = 1;
    entry->cache_disabled = 1;

    entry = &pages[1].entries[0];
    entry->present = 1;
    entry->pfn = _translate(&pages[2]) >> PAGE_BITS;
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

#elif (defined(ARCH_IA32))

static struct page_frame page aligned_var(PAGE_SIZE);
static struct page_frame *page_table_root = &page;

static void setup_boot_page_table()
{
    u64 cur_paddr = 0;
    for (int i = 0; i < PAGE_TABLE_ENTRY_COUNT;
         i++, cur_paddr += 0x400000ull
    ) {
        struct page_table_entry *entry = &page.entries[i];
        entry->value = 0;

        entry->present = 1;
        entry->pfn = cur_paddr >> PAGE_BITS;
        entry->write_allow = 1;
        entry->cache_disabled = 1;
        entry->large_page = 1;
    }
}

#endif

void init_mp_boot()
{
    ulong size = (ulong)&_mp_boot_end - (ulong)&_mp_boot_start;
    memcpy((void *)0x8000, &_mp_boot_start, size);

    struct hal_arch_funcs *funcs = get_hal_arch_funcs();
    ulong *loader_entry = (void *)0x7000ul;
    *loader_entry = funcs->mp_entry;
    funcs->mp_entry = 0x8000;

    setup_boot_page_table();
    ulong *boot_page_table = (void *)0x7008ul;
    *boot_page_table = _translate(page_table_root);

    kprintf("Boot page table @ %lx\n", *boot_page_table);
}
