#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "loader/include/lib.h"
#include "loader/include/mem.h"
#include "loader/include/devtree.h"
#include "loader/include/firmware.h"
#include "loader/include/kprintf.h"


/*
 * DEC processor types for Alpha systems
 */
enum srm_cpu_type {
    SRM_CPU_TYPE_EV3        = 1,       // EV3
    SRM_CPU_TYPE_EV4        = 2,       // EV4 (21064)
    SRM_CPU_TYPE_LCA4       = 4,       // LCA4 (21066/21068)
    SRM_CPU_TYPE_EV5        = 5,       // EV5 (21164)
    SRM_CPU_TYPE_EV45       = 6,       // EV4.5 (21064/xxx)
    SRM_CPU_TYPE_EV56       = 7,       // EV5.6 (21164)
    SRM_CPU_TYPE_EV6        = 8,       // EV6 (21264)
    SRM_CPU_TYPE_PCA56      = 9,       // PCA56 (21164PC)
    SRM_CPU_TYPE_PCA57      = 10,      // PCA57 (notyet)
    SRM_CPU_TYPE_EV67       = 11,      // EV67 (21264A)
    SRM_CPU_TYPE_EV68CB     = 12,      // EV68CB (21264C)
    SRM_CPU_TYPE_EV68AL     = 13,      // EV68AL (21264B)
    SRM_CPU_TYPE_EV68CX     = 14,      // EV68CX (21264D)
    SRM_CPU_TYPE_EV7        = 15,      // EV7 (21364)
    SRM_CPU_TYPE_EV79       = 16,      // EV79 (21364??)
    SRM_CPU_TYPE_EV69       = 17,      // EV69 (21264/EV69A)
};

/*
 * DEC system types for Alpha systems
 */
#define ST_ADU              1       // Alpha ADU systype
#define ST_DEC_4000         2       // Cobra systype
#define ST_DEC_7000         3       // Ruby systype
#define ST_DEC_3000_500     4       // Flamingo systype
#define ST_DEC_2000_300     6       // Jensen systype
#define ST_DEC_3000_300     7       // Pelican systype
#define ST_DEC_2100_A500    9       // Sable systype
#define ST_DEC_AXPVME_64    10      // AXPvme system type
#define ST_DEC_AXPPCI_33    11      // NoName system type
#define ST_DEC_TLASER       12      // Turbolaser systype
#define ST_DEC_2100_A50     13      // Avanti systype
#define ST_DEC_MUSTANG      14      // Mustang systype
#define ST_DEC_ALCOR        15      // Alcor (EV5) systype
#define ST_DEC_1000         17      // Mikasa systype
#define ST_DEC_EB64         18      // EB64 systype
#define ST_DEC_EB66         19      // EB66 systype
#define ST_DEC_EB64P        20      // EB64+ systype
#define ST_DEC_BURNS        21      // laptop systype
#define ST_DEC_RAWHIDE      22      // Rawhide systype
#define ST_DEC_K2           23      // K2 systype
#define ST_DEC_LYNX         24      // Lynx systype
#define ST_DEC_XL           25      // Alpha XL systype
#define ST_DEC_EB164        26      // EB164 systype
#define ST_DEC_NORITAKE     27      // Noritake systype
#define ST_DEC_CORTEX       28      // Cortex systype
#define ST_DEC_MIATA        30      // Miata systype
#define ST_DEC_XXM          31      // XXM systype
#define ST_DEC_TAKARA       32      // Takara systype
#define ST_DEC_YUKON        33      // Yukon systype
#define ST_DEC_TSUNAMI      34      // Tsunami systype
#define ST_DEC_WILDFIRE     35      // Wildfire systype
#define ST_DEC_CUSCO        36      // CUSCO systype
#define ST_DEC_EIGER        37      // Eiger systype
#define ST_DEC_TITAN        38      // Titan systype
#define ST_DEC_MARVEL       39      // Marvel systype

#define ST_UNOFFICIAL_BIAS  100     // Unofficial
#define ST_DTI_RUFFIAN      101     // RUFFIAN systype

#define ST_API_BIAS         200     // Alpha Processor, Inc. systems
#define ST_API_NAUTILUS     201     // UP1000 systype

/*
 * Console callback routine numbers
 */
#define CCB_GETC            0x01
#define CCB_PUTS            0x02
#define CCB_RESET_TERM      0x03
#define CCB_SET_TERM_INT    0x04
#define CCB_SET_TERM_CTL    0x05
#define CCB_PROCESS_KEYCODE 0x06
#define CCB_OPEN_CONSOLE    0x07
#define CCB_CLOSE_CONSOLE   0x08

#define CCB_OPEN            0x10
#define CCB_CLOSE           0x11
#define CCB_IOCTL           0x12
#define CCB_READ            0x13
#define CCB_WRITE           0x14

#define CCB_SET_ENV         0x20
#define CCB_RESET_ENV       0x21
#define CCB_GET_ENV         0x22
#define CCB_SAVE_ENV        0x23

#define CCB_PSWITCH         0x30
#define CCB_BIOS_EMUL       0x32

/*
 * Environment variable numbers
 */
#define ENV_AUTO_ACTION     0x01
#define ENV_BOOT_DEV        0x02
#define ENV_BOOTDEF_DEV     0x03
#define ENV_BOOTED_DEV      0x04
#define ENV_BOOT_FILE       0x05
#define ENV_BOOTED_FILE     0x06
#define ENV_BOOT_OSFLAGS    0x07
#define ENV_BOOTED_OSFLAGS  0x08
#define ENV_BOOT_RESET      0x09
#define ENV_DUMP_DEV        0x0A
#define ENV_ENABLE_AUDIT    0x0B
#define ENV_LICENSE         0x0C
#define ENV_CHAR_SET        0x0D
#define ENV_LANGUAGE        0x0E
#define ENV_TTY_DEV         0x0F


/*
 * HWRPB structures
 */
struct srm_per_cpu {
    u64 hwpcb[16];
    u64 flags;
    u64 pal_mem_size;
    u64 pal_scratch_size;
    u64 pal_mem_pa;
    u64 pal_scratch_pa;
    u64 pal_revision;
    u64 type;
    u64 variation;
    u64 revision;
    u64 serial_no[2];
    u64 logout_area_pa;
    u64 logout_area_len;
    u64 halt_PCBB;
    u64 halt_PC;
    u64 halt_PS;
    u64 halt_arg;
    u64 halt_ra;
    u64 halt_pv;
    u64 halt_reason;
    u64 res;
    u64 ipc_buffer[21];
    u64 palcode_avail[16];
    u64 compatibility;
    u64 console_data_log_pa;
    u64 console_data_log_length;
    u64 bcache_info;
} packed8_struct;

struct srm_proc_desc {
    u64 weird_vms_stuff;
    u64 address;
} packed8_struct;

struct srm_vf_map {
    u64 va;
    u64 pa;
    u64 count;
} packed8_struct;

struct srm_crb {
    struct srm_proc_desc *dispatch_va;
    struct srm_proc_desc *dispatch_pa;
    struct srm_proc_desc *fixup_va;
    struct srm_proc_desc *fixup_pa;

    // virtual->physical map
    u64 map_entries;
    u64 map_pages;
    struct srm_vf_map map[1];
} packed8_struct;

struct srm_mem_cluster {
    u64 start_pfn;
    u64 num_pages;
    u64 num_tested;
    u64 bitmap_va;
    u64 bitmap_pa;
    u64 bitmap_chksum;
    u64 usage;
} packed8_struct;

struct srm_mem_desc {
    u64 chksum;
    u64 optional_pa;
    u64 num_clusters;
    struct srm_mem_cluster clusters[0];
} packed8_struct;

struct srm_dsr {
    s64 smm;                // SMM nubber used by LMF
    u64 lurt_off;           // Offset to LURT table
    u64 sysname_off;        // Offset to sysname char count
} packed8_struct;

struct srm_hwrbp {
    u64 paddr;              // Must be HWRPB physical address
    u8  magic[8];           // "HWRPB\0\0\0"
    u64 revision;           // HWRPB revision
    u64 size;               // HWRPB size
    u64 cpuid;              // Primary CPU ID
    u64 page_size;          // 8192 is the only supported page size
    u32 num_pa_bits;        // Number of physical address bits
    u32 num_ext_va_bits;    // Number of extension virtual address bits
    u64 max_asn;            // Max valid ASN
    u8  ssn[16];            // System serial number
    u64 sys_type;           // System type
    u64 sys_variation;      // System variation
    u64 sys_revision;       // System revision
    u64 intr_freq;          // Interval clock frequency * 4096
    u64 cycle_freq;         // Cycle counter frequency
    u64 vptb;               // Virtual page table base
    u64 res1;               // Reserved for architecture
    u64 tbhb_offset;        // Offset to translation buffer hint block
    u64 nr_processors;      // Num of processor slots
    u64 processor_size;     // Per-CPU slot size
    u64 processor_offset;   // Offset to per-CPU slots
    u64 ctb_nr;             // Number of CTBs
    u64 ctb_size;           // CTB size
    u64 ctbt_offset;        // Offset to console terminal block table (CTB)
    u64 crb_offset;         // Offset to console callback routine block (CRB)
    u64 mddt_offset;        // Offset to memory data descriptor table (MDDT)
    u64 cdb_offset;         // Offset to configuration data block (CDB)
    u64 frut_offset;        // Offset to FRU table (FRUT)
    u64 save_term_vaddr;    // Terminal save state routine
    u64 save_term_data;     // Terminal save state procedure value
    u64 restore_term_vaddr; // Terminal restore state routine
    u64 restore_term_data;  // Terminal restore state procedure value
    u64 cpu_restart_vaddr;  // CPU restart routine
    u64 cpu_restart_data;   // CPU restart routine procedure value
    u64 res2;               // Reserved for system software
    u64 res3;               // Reserved for hardware
    u64 chksum;             // Checksum
    u64 rxrdy;              // RX
    u64 txrdy;              // TX
    u64 dsr_offset;         // Offset to dynamic system recognition data block table (DSR)
} packed8_struct;


static struct srm_hwrbp *hwrpb = NULL;


typedef long (*dispatch3_t)(ulong proc, ulong a1, ulong a2, ulong a3);

static long dispatch3(ulong proc, ulong a1, ulong a2, ulong a3)
{
    if (!hwrpb) {
        return -1;
    }

    struct srm_crb *crb = (void *)hwrpb + hwrpb->crb_offset;
    dispatch3_t disp = (void *)crb->dispatch_va->address;
    return disp(proc, a1, a2, a3);
}

static int get_env(int index, char *val, int size)
{
    static char env_buf[257] aligned_var(8);

    int len = dispatch3(CCB_GET_ENV, index, (ulong)env_buf, 256);
    if (len >= 0) {
        env_buf[len] = '\0';
        kprintf("SRM env: %s\n", env_buf);
        //if (size < len + 1) {
        //    len = size - 1;
        //}
        //memcpy(val, env_buf, len);
        //val[len] = '\0';
    }

    return len;
}

static void add_bootarg(char *cmdline)
{
    //struct firmware_args *fw_args = get_fw_args();
    //char *cmdline = fw_args->srm.cmdline;
    if (cmdline && *cmdline) {
        kprintf("bootarg: %s\n", cmdline);
    }
}

static inline char num_to_hex_digit(u32 num, int idx)
{
    u32 mask = 0xf << (idx * 4);
    num &= mask;
    num >>= idx * 4;
    return num >= 10 ? 'a' + num - 10 : '0' + num;
}

static void add_memmap()
{
    struct srm_mem_desc *memdesc = (void *)hwrpb + hwrpb->mddt_offset;
    kprintf("hwrpb @ %p, num clusters: %lld\n", hwrpb, memdesc->num_clusters);

    int num_usable_clusters = 0;
    u64 mem_start = -0x1ull;
    u64 mem_end = 0;

    for (u64 i = 0; i < memdesc->num_clusters; i++) {
        struct srm_mem_cluster *cluster = &memdesc->clusters[i];

        u64 start_addr = ppfn_to_paddr(cluster->start_pfn);
        u64 end_addr = ppfn_to_paddr(cluster->start_pfn + cluster->num_pages);
        if (start_addr < mem_start) mem_start = start_addr;
        if (end_addr > mem_end) mem_end = end_addr;

        if (!cluster->usage) {
            // Usage: 0 = usable, 1 = reserved, 2 = volatile
            num_usable_clusters++;
        }

        kprintf("cluster %lld, start ppn: %llx, num pages: %lld, usage: %llx\n",
                i, cluster->start_pfn, cluster->num_pages, cluster->usage);
    }

    // Get chosen node
    struct devtree_node *chosen = devtree_walk("/chosen");
    if (!chosen) {
        chosen = devtree_alloc_node(devtree_get_root(), "chosen");
    }

    devtree_alloc_prop_u64(chosen, "memstart", mem_start);
    devtree_alloc_prop_u64(chosen, "memsize", mem_end - mem_start);

    // Get memrsv-block node
    struct devtree_node *memsrv = devtree_walk("/memrsv-block");
    if (!memsrv) {
        memsrv = devtree_alloc_node(devtree_get_root(), "memrsv-block");
    }

    u32 prop_idx = 0;
    for (u64 i = 0; i < memdesc->num_clusters; i++) {
        struct srm_mem_cluster *cluster = &memdesc->clusters[i];

        // Usage: 0 = usable, 1 = reserved, 2 = volatile
        if (cluster->usage) {
            u64 start_addr = ppfn_to_paddr(cluster->start_pfn);
            u64 end_addr = ppfn_to_paddr(cluster->start_pfn + cluster->num_pages);
            u64 size = end_addr - start_addr;

            char prop_name[] = {
                    num_to_hex_digit(prop_idx, 6),
                    num_to_hex_digit(prop_idx, 5),
                    num_to_hex_digit(prop_idx, 4),
                    num_to_hex_digit(prop_idx, 3),
                    num_to_hex_digit(prop_idx, 2),
                    num_to_hex_digit(prop_idx, 1),
                    num_to_hex_digit(prop_idx, 0),
                    '\0' };
            u64 prop_data[] = {
                    swap_big_endian64(start_addr),
                    swap_big_endian64(size) };
            devtree_alloc_prop(memsrv, prop_name, prop_data, 16);

            prop_idx++;
        }
    }
}

static void add_initrd(void *initrd_start, ulong initrd_size)
{
    // Get chosen node
    struct devtree_node *chosen = devtree_walk("/chosen");
    if (!chosen) {
        chosen = devtree_alloc_node(devtree_get_root(), "chosen");
    }

    // Initrd
    u64 initrd_start64 = (u64)(ulong)initrd_start;
    u64 initrd_size64 = (u64)initrd_size;
    u64 initrd_end64 = initrd_start64 + initrd_size64;

    if (initrd_start && initrd_size) {
        devtree_alloc_prop_u64(chosen, "initrd-start", initrd_start64);
        devtree_alloc_prop_u64(chosen, "initrd-end", initrd_end64);
    }
}

static void display_srm_info()
{
    kprintf("HWRPB vaddr @ %p, paddr @ %llx, size: %llu\n",
            hwrpb, hwrpb->paddr, hwrpb->size);

    struct srm_per_cpu *spc = (void *)((ulong)hwrpb + hwrpb->processor_offset);
    kprintf("SRM num processors: %llu, per CPU offset: %llu, vaddr @ %p, size: %llu, "
            "PAL paddr @ %llx, size: %llu, PAL scratch paddr @ %llx, size: %llu\n",
            hwrpb->nr_processors, hwrpb->processor_offset, spc, hwrpb->processor_size,
            spc->pal_mem_pa, spc->pal_mem_size, spc->pal_scratch_pa, spc->pal_scratch_size);
    kprintf("%s, %llu, %llu, %llu\n", spc->hwpcb, spc->type, spc->revision, spc->variation);

    //while (1);
}

static void init_srm(void *params)
{
    struct firmware_params_srm *srm = params;
    hwrpb = srm->hwrpb_base;
    display_srm_info();

    add_bootarg(srm->cmdline);
    get_env(0, NULL, 0);
    add_initrd(srm->initrd_start, srm->initrd_size);
    add_memmap();
}


DECLARE_FIRMWARE_DRIVER(srm) = {
    .name = "srm",
    .need_fdt = 1,
    .init = init_srm,
};
