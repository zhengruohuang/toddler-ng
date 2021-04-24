#include "common/include/inttypes.h"
#include "common/include/compiler.h"
#include "common/include/gdt.h"
#include "common/include/tss.h"
#include "common/include/msr.h"
#include "hal/include/lib.h"
#include "hal/include/mem.h"
#include "hal/include/mp.h"


/*
 * GDT
 */
enum gdt_index {
    GDT_INDEX_START = 0,

    GDT_INDEX_KERNEL_CODE,
    GDT_INDEX_KERNEL_DATA,

    GDT_INDEX_USER_CODE,
    GDT_INDEX_USER_DATA,

    GDT_INDEX_TSS,

#if (ARCH_WIDTH == 64)
    GDT_INDEX_TSS64,

    NUM_GDT_ENTRIES,

    GDT_INDEX_KERNEL_MP_SEQ = GDT_INDEX_KERNEL_DATA,
    GDT_INDEX_USER_MP_TLS_PTR = GDT_INDEX_USER_DATA,

#elif (ARCH_WIDTH == 32)
    GDT_INDEX_KERNEL_MP_SEQ,
    GDT_INDEX_USER_MP_TLS_PTR,

    NUM_GDT_ENTRIES,

#endif
};

static inline void build_gdt_entry(struct global_desc_table_entry *entry,
                                   ulong base, ulong limit)
{
    memzero(entry, sizeof(struct global_desc_table_entry));

    entry->limit_low = ((u16)limit) & 0xffff;
    entry->limit_high = (u8)(limit >> 16);
    entry->base_low = ((u16)base) & 0xffff;
    entry->base_mid = ((u8)(base >> 16)) & 0xff;
    entry->base_high = ((u8)(base >> 24)) & 0xff;
}

static void build_gdt_code_data(struct global_desc_table_entry *entry,
                                ulong base, ulong limit, int granu4k,
                                int code, int user)
{
    build_gdt_entry(entry, base, limit);
    entry->read_write = 1;
    entry->exec = code ? 1 : 0;
    entry->type = 1;
    entry->privilege = user ? 3 : 0;
    entry->present = 1;
    entry->size = ARCH_WIDTH == 64 ? 0 : 1;
    entry->granularity = granu4k;

#if (ARCH_WIDTH == 64)
    entry->amd64_seg = code ? 1 : 0;
#endif
}

static void build_gdt_tss(struct global_desc_table_entry *entry,
                          struct task_state_segment *tss)
{
    ulong base = (ulong)(void *)tss;
    ulong limit = sizeof(struct task_state_segment);

    build_gdt_entry(entry, base, limit);
    entry->accessed = 1;
    entry->exec = 1;
    entry->present = 1;

#if (ARCH_WIDTH == 64)
    entry->system = 1;

    struct global_desc_table_entry *entry64 = entry + 1;
    entry64->tss64_base.high32 = (u32)(base >> 32);
    entry64->tss64_base.zero = 0;
#endif
}

#if (ARCH_WIDTH == 32)
static void build_gdt_kernel_mp_seq(struct global_desc_table_entry *entry,
                                u32 *mp_seq)
{
    ulong base = (ulong)(void *)mp_seq;
    ulong limit = 16;

    build_gdt_code_data(entry, base, limit, 0, 0, 0);
}

static void build_gdt_user_mp_tls_ptr(struct global_desc_table_entry *entry,
                                      ulong *tls_ptr)
{
    ulong base = (ulong)(void *)tls_ptr;
    ulong limit = 16;

    build_gdt_code_data(entry, base, limit, 0, 0, 1);
}
#endif


/*
 * Segment selectors
 */
static struct segment_selector seg_selectors[NUM_GDT_ENTRIES];

static inline void build_seg_sel(int idx, int user)
{
    struct segment_selector *sel = &seg_selectors[idx];
    sel->value = 0;
    sel->index = idx;
    sel->privilege = user ? 0x3 : 0;
}

static inline u16 get_selector(int idx)
{
    return seg_selectors[idx].value;
}

u16 get_segment_selector(char type, int user_mode)
{
    switch (type) {
    case 'c':
        return get_selector(user_mode ? GDT_INDEX_USER_CODE : GDT_INDEX_KERNEL_CODE);
    case 'd':
    case 'e':
    case 's':
        return get_selector(user_mode ? GDT_INDEX_USER_DATA : GDT_INDEX_KERNEL_DATA);
    case 'f':
        return get_selector(GDT_INDEX_USER_MP_TLS_PTR);
    case 'g':
        return get_selector(user_mode ? GDT_INDEX_USER_DATA : GDT_INDEX_KERNEL_MP_SEQ);
    default:
        break;
    }

    panic("Unknown segment type: %c!\n", type);
    return 0;
}

u16 get_kernel_code_selector()
{
    return get_selector(GDT_INDEX_KERNEL_CODE);
}


/*
 * Per-CPU kernel segment tables
 */
struct segment_table {
    u32 mp_seq;
    u32 pad[3];
    struct global_desc_table_entry gdt[NUM_GDT_ENTRIES];
    struct task_state_segment tss;
    struct global_desc_table_ptr gdtr;
} packed8_struct;

static ulong seg_table_size = 0;
static int num_seg_tables = 0;

static paddr_t seg_tables_paddr = 0;
static ulong seg_tables_vaddr = 0;

static inline struct segment_table *get_seg_table(int mp_seq)
{
    ulong vaddr = seg_tables_vaddr + mp_seq * seg_table_size;
    return (struct segment_table *)vaddr;
}


/*
 * Per-CPU user TLS ptrs
 */
struct user_tls_ptr {
    ulong tls;
} natural_struct;

static ulong user_tls_ptr_size = 0;
static int num_user_tls_ptrs = 0;

static paddr_t user_tls_ptrs_paddr = 0;
static ulong user_tls_ptrs_vaddr = 0;

static inline struct user_tls_ptr *get_user_tls_ptr(int mp_seq)
{
    ulong vaddr = user_tls_ptrs_vaddr + mp_seq * user_tls_ptr_size;
    return (struct user_tls_ptr *)vaddr;
}

void set_user_tls_ptr(int mp_seq, ulong tls)
{
    struct user_tls_ptr *tls_ptr = get_user_tls_ptr(mp_seq);
    tls_ptr->tls = tls;
}


/*
 * Load TSS, GDT, and seg registers
 */
void load_tss_mp(int mp_seq, ulong sp)
{
    panic_if(mp_seq >= num_seg_tables, "Running out of segment tables!\n");
    struct segment_table *table = get_seg_table(mp_seq);

#if (ARCH_WIDTH == 32)
    table->tss.ss0 = get_selector(GDT_INDEX_KERNEL_DATA);
#endif
    table->tss.sp0 = sp;
    table->tss.iopb_addr = 0;

    __asm__ __volatile__ (
        "ltr %%ax;"
        :
        : "a" (get_selector(GDT_INDEX_TSS))
        : "memory", "cc"
    );
}

void load_gdt_mp(int mp_seq)
{
    panic_if(mp_seq >= num_seg_tables, "Running out of segment tables!\n");
    struct segment_table *table = get_seg_table(mp_seq);

    struct global_desc_table_ptr *gdtr = &table->gdtr;
    gdtr->base = (ulong)(void *)&table->gdt;
    gdtr->limit = sizeof(struct global_desc_table_entry) * NUM_GDT_ENTRIES;

    __asm__ __volatile__ (
        "   lgdt   (%[gdtr]);"
        "   jmp    1f;"         // Force to use the new GDT
        "1: nop; nop;"
        "   movw   %%ax, %%ds;"
        "   movw   %%ax, %%es;"
        "   movw   %%cx, %%fs;"
        "   movw   %%dx, %%gs;"
        "   movw   %%ax, %%ss;"
        :
        : [gdtr] "b" (gdtr),
          "a" (get_selector(GDT_INDEX_KERNEL_DATA)),
          "c" (get_selector(GDT_INDEX_USER_MP_TLS_PTR)),
          "d" (get_selector(GDT_INDEX_KERNEL_MP_SEQ))
        : "memory", "cc"
    );

#if (ARCH_WIDTH == 64)
    struct user_tls_ptr *tls_ptr = get_user_tls_ptr(mp_seq);
    write_fsbase(tls_ptr);
    write_gsbase(&table->mp_seq);
    write_gsbase_kernel(&table->mp_seq);
#endif
}

void load_program_seg(ulong es, ulong fs, ulong gs)
{
    // DS and SS are treated specially so that they are not reloaded here
    __asm__ __volatile__ (
        "   movw   %%ax, %%es;"
        "   movw   %%cx, %%fs;"
        "   movw   %%dx, %%gs;"
        :
        : "a" (es), "c" (fs), "d" (gs)
        : "memory", "cc"
    );

#if (ARCH_WIDTH == 64)
    // Load kernel GS
    ulong kernel_gs;
    read_gsbase_kernel(kernel_gs);
    write_gsbase(kernel_gs);

    // Load user FS
    int mp_seq = get_cur_bringup_mp_seq();
    struct user_tls_ptr *tls_ptr = get_user_tls_ptr(mp_seq);
    write_fsbase(tls_ptr);
#endif
}

void load_kernel_seg()
{
    load_program_seg(get_selector(GDT_INDEX_KERNEL_DATA),
                     get_selector(GDT_INDEX_USER_MP_TLS_PTR),
                     get_selector(GDT_INDEX_KERNEL_MP_SEQ));
}


/*
 * Init
 */
static void init_kernel_seg_table()
{
    seg_table_size = sizeof(struct segment_table);
    seg_table_size = align_up_vsize(seg_table_size, 16);

    ulong tables_size = seg_table_size * get_num_cpus();
    tables_size = align_up_vsize(tables_size, PAGE_SIZE);
    num_seg_tables = tables_size / seg_table_size;

    ulong tables_page_count = tables_size / PAGE_SIZE;
    ppfn_t seg_tables_ppfn = pre_palloc(tables_page_count);
    seg_tables_paddr = ppfn_to_paddr(seg_tables_ppfn);
    seg_tables_vaddr = pre_valloc(tables_page_count, seg_tables_paddr, 1, 1);
}

static void init_user_tls_ptr_table()
{
    user_tls_ptr_size = sizeof(struct user_tls_ptr);
    user_tls_ptr_size = align_up_vsize(user_tls_ptr_size, 16);

    ulong tables_size = user_tls_ptr_size * get_num_cpus();
    tables_size = align_up_vsize(tables_size, PAGE_SIZE);
    num_user_tls_ptrs = tables_size / user_tls_ptr_size;

    ulong tables_page_count = tables_size / PAGE_SIZE;
    ppfn_t user_tls_ptrs_ppfn = pre_palloc(tables_page_count);
    user_tls_ptrs_paddr = ppfn_to_paddr(user_tls_ptrs_ppfn);
    user_tls_ptrs_vaddr = pre_valloc(tables_page_count, user_tls_ptrs_paddr, 1, 0);
}

static void build_kernel_seg_table()
{
    for (int i = 0; i < num_seg_tables; i++) {
        struct segment_table *table = get_seg_table(i);
        memzero(table, sizeof(struct segment_table));

        struct user_tls_ptr *tls_ptr = get_user_tls_ptr(i);
        memzero(tls_ptr, sizeof(struct user_tls_ptr));

        table->mp_seq = i;
        build_gdt_entry(&table->gdt[GDT_INDEX_START], 0, 0);
        build_gdt_code_data(&table->gdt[GDT_INDEX_KERNEL_CODE], 0, 0xfffff, 1, 1, 0);
        build_gdt_code_data(&table->gdt[GDT_INDEX_KERNEL_DATA], 0, 0xfffff, 1, 0, 0);
        build_gdt_code_data(&table->gdt[GDT_INDEX_USER_CODE], 0, 0xfffff, 1, 1, 1);
        build_gdt_code_data(&table->gdt[GDT_INDEX_USER_DATA], 0, 0xfffff, 1, 0, 1);
        build_gdt_tss(&table->gdt[GDT_INDEX_TSS], &table->tss);

#if (ARCH_WIDTH == 32)
        build_gdt_kernel_mp_seq(&table->gdt[GDT_INDEX_KERNEL_MP_SEQ], &table->mp_seq);
        build_gdt_user_mp_tls_ptr(&table->gdt[GDT_INDEX_USER_MP_TLS_PTR], &tls_ptr->tls);
#endif
    }
}

static void build_seg_registers()
{
    build_seg_sel(GDT_INDEX_KERNEL_CODE, 0);
    build_seg_sel(GDT_INDEX_KERNEL_DATA, 0);
    build_seg_sel(GDT_INDEX_USER_CODE, 1);
    build_seg_sel(GDT_INDEX_USER_DATA, 1);
    build_seg_sel(GDT_INDEX_TSS, 0);

#if (ARCH_WIDTH == 32)
    build_seg_sel(GDT_INDEX_KERNEL_MP_SEQ, 0);
    build_seg_sel(GDT_INDEX_USER_MP_TLS_PTR, 1);
#endif
}

void init_segment()
{
    init_kernel_seg_table();
    init_user_tls_ptr_table();
    build_kernel_seg_table();
    build_seg_registers();
}
