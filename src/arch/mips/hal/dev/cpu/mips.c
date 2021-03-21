#include "common/include/inttypes.h"
#include "common/include/msr.h"
#include "loader/include/args.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/hal.h"
#include "hal/include/lib.h"
#include "hal/include/dev.h"
#include "hal/include/mp.h"


struct mips_cpu {
    ulong mp_id;
    struct mips_cpu *next;
};


/*
 * Topology
 */
static struct mips_cpu *cpus = NULL;

static void detect_topology(struct driver_param *param)
{
    struct loader_args *largs = get_loader_args();
    struct mips_loader_args *mips_args = largs->arch_args;

    u8 *bool_array = mips_args->cpunum_table.bool_array;
    int num_entries = mips_args->cpunum_table.num_entries;

    // MIPS64 QEMU is not multi-threaded, therefore it's possible that other
    // CPUs haven't initialized the cpunum table yet. The delay loop slows down
    // current CPU a bit, and therefore gives other CPUs chance to initialize
    // cpunum table. This is a temp hack. Eventually will be replaced by GIC.
#if (ARCH_WIDTH == 64)
    kprintf("bool array @ %lx\n", bool_array);
    for (volatile ulong i = 0; i < 10000; i++) {
        for (volatile ulong j = 0; j < 10000; j++);
    }
#endif

    // Count num of CPUs
    ulong num_cpus = 0;
    for (int i = 0; i < num_entries; i++) {
        if (bool_array[i]) {
            num_cpus++;
        }
    }
    set_num_cpus(num_cpus);

    // Set up translation
    ulong boot_mp_id = arch_get_cur_mp_id();
    set_mp_trans(0, boot_mp_id);

    for (int i = 0, mp_seq = 1; i < num_entries; i++) {
        ulong mp_id = i;
        int valid = bool_array[i];
        if (valid && mp_id != boot_mp_id) {
            set_mp_trans(mp_seq, mp_id);
            mp_seq++;
        }
    }

    // Finalize
    final_mp_trans();
}


/*
 * Driver interface
 */
static void setup(struct driver_param *param)
{
    struct mips_cpu *record = param->record;

    ulong mp_id = arch_get_cur_mp_id();
    if (record->mp_id == mp_id) {
        REG_SPECIAL_DRV_FUNC(detect_topology, param, detect_topology);
    }
}

static int probe(struct fw_dev_info *fw_info, struct driver_param *param)
{
    static const char *devtree_names[] = {
        "img,mips", "mips",
        NULL
    };

    if (fw_info->devtree_node &&
        match_devtree_compatibles(fw_info->devtree_node, devtree_names)
    ) {
        struct mips_cpu *record = mempool_alloc(sizeof(struct mips_cpu));
        memzero(record, sizeof(struct mips_cpu));
        param->record = record;

        struct devtree_prop *prop = devtree_find_prop(fw_info->devtree_node, "reg");
        panic_if(!prop, "Bad OR1200 CPU devtree node!\n");

        record->mp_id = devtree_get_prop_data_u32(prop);
        record->next = cpus;
        cpus = record;

        kprintf("Found MIPS CPU\n");
        return FW_DEV_PROBE_OK;
    }

    return FW_DEV_PROBE_FAILED;
}


DECLARE_DEV_DRIVER(mips_cpu) = {
    .name = "MIPS CPU",
    .probe = probe,
    .setup = setup,
};
