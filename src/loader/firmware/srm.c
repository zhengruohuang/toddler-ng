#include "common/include/inttypes.h"
#include "loader/include/lib.h"
#include "loader/include/devtree.h"
#include "loader/include/firmware.h"
#include "loader/include/kprintf.h"


#define PARAM_OFFSET -0x6000
#define HWRPB_BASE ((struct srm_hwrbp *)0x10000000ul)

/*
 * DEC processor types for Alpha systems
 */
#define EV3_CPU         1       // EV3
#define EV4_CPU         2       // EV4 (21064)
#define LCA4_CPU        4       // LCA4 (21066/21068)
#define EV5_CPU         5       // EV5 (21164)
#define EV45_CPU        6       // EV4.5 (21064/xxx)
#define EV56_CPU        7       // EV5.6 (21164)
#define EV6_CPU         8       // EV6 (21264)
#define PCA56_CPU       9       // PCA56 (21164PC)
#define PCA57_CPU       10      // PCA57 (notyet)
#define EV67_CPU        11      // EV67 (21264A)
#define EV68CB_CPU      12      // EV68CB (21264C)
#define EV68AL_CPU      13      // EV68AL (21264B)
#define EV68CX_CPU      14      // EV68CX (21264D)
#define EV7_CPU         15      // EV7 (21364)
#define EV79_CPU        16      // EV79 (21364??)
#define EV69_CPU        17      // EV69 (21264/EV69A)

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
struct srm_pcb {
    ulong ksp;
    ulong usp;
    ulong ptbr;
    uint pcc;
    uint asn;
    ulong unique;
    ulong flags;
    ulong res1, res2;
} packed_struct;

struct srm_per_cpu {
    ulong hwpcb[16];
    ulong flags;
    ulong pal_mem_size;
    ulong pal_scratch_size;
    ulong pal_mem_pa;
    ulong pal_scratch_pa;
    ulong pal_revision;
    ulong type;
    ulong variation;
    ulong revision;
    ulong serial_no[2];
    ulong logout_area_pa;
    ulong logout_area_len;
    ulong halt_PCBB;
    ulong halt_PC;
    ulong halt_PS;
    ulong halt_arg;
    ulong halt_ra;
    ulong halt_pv;
    ulong halt_reason;
    ulong res;
    ulong ipc_buffer[21];
    ulong palcode_avail[16];
    ulong compatibility;
    ulong console_data_log_pa;
    ulong console_data_log_length;
    ulong bcache_info;
} packed_struct;

struct srm_proc_desc {
    ulong weird_vms_stuff;
    ulong address;
} packed_struct;

struct srm_vf_map {
    ulong va;
    ulong pa;
    ulong count;
} packed_struct;

struct srm_crb {
    struct srm_proc_desc *dispatch_va;
    struct srm_proc_desc *dispatch_pa;
    struct srm_proc_desc *fixup_va;
    struct srm_proc_desc *fixup_pa;

    /* virtual->physical map */
    ulong map_entries;
    ulong map_pages;
    struct srm_vf_map map[1];
} packed_struct;

struct srm_mem_clust {
    ulong start_pfn;
    ulong num_pages;
    ulong num_tested;
    ulong bitmap_va;
    ulong bitmap_pa;
    ulong bitmap_chksum;
    ulong usage;
} packed_struct;

struct srm_mem_desc {
    ulong chksum;
    ulong optional_pa;
    ulong num_clusters;
    struct srm_mem_clust clusters[0];
} packed_struct;

struct srm_dsr {
    long smm;           // SMM nubber used by LMF
    ulong lurt_off;     // offset to LURT table
    ulong sysname_off;  // offset to sysname char count
} packed_struct;

struct srm_hwrbp {
    ulong phys_addr;    // check: physical address of the hwrpb
    ulong id;           // check: "HWRPB\0\0\0"
    ulong revision;
    ulong size;         // size of hwrpb
    ulong cpuid;
    ulong pagesize;     // 8192, I hope
    ulong pa_bits;      // number of physical address bits
    ulong max_asn;
    uchar ssn[16];      // system serial number: big bother is watching
    ulong sys_type;
    ulong sys_variation;
    ulong sys_revision;
    ulong intr_freq;    // interval clock frequency * 4096
    ulong cycle_freq;   // cycle counter frequency
    ulong vptb;         // Virtual Page Table Base address
    ulong res1;
    ulong tbhb_offset;  // Translation Buffer Hint Block
    ulong nr_processors;
    ulong processor_size;
    ulong processor_offset;
    ulong ctb_nr;
    ulong ctb_size;     // console terminal block size
    ulong ctbt_offset;  // console terminal block table offset
    ulong crb_offset;   // console callback routine block
    ulong mddt_offset;  // memory data descriptor table
    ulong cdb_offset;   // configuration data block (or NULL)
    ulong frut_offset;  // FRU table (or NULL)
    void (*save_terminal)(ulong);
    ulong save_terminal_data;
    void (*restore_terminal)(ulong);
    ulong restore_terminal_data;
    void (*cpu_restart)(ulong);
    ulong cpu_restart_data;
    ulong res2;
    ulong res3;
    ulong chksum;
    ulong rxrdy;
    ulong txrdy;
    ulong dsr_offset;   // "Dynamic System Recognition Data Block Table"
} packed_struct;


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

static char env_buf[256] aligned_var(8);

static int get_env(int index, char *val, int size)
{
    int len = dispatch3(CCB_GET_ENV, index, (ulong)env_buf, 256);
    if (len >= 0) {
        if (size < len + 1) {
            len = size - 1;
        }
        memcpy(val, env_buf, len);
        val[len] = '\0';
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

static void add_memmap()
{
}


static void srm_add_initrd(void *initrd_start, ulong initrd_size)
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

static void init_srm(void *params)
{
    struct firmware_params_srm *srm = params;
    hwrpb = srm->hwrpb_base;

    add_bootarg(srm->cmdline);
    add_memmap();
    srm_add_initrd(srm->initrd_start, srm->initrd_size);
}


DECLARE_FIRMWARE_DRIVER(srm) = {
    .name = "srm",
    .init = init_srm,
};
