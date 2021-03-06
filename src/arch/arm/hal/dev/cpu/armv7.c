#include "common/include/inttypes.h"
#include "common/include/msr.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/hal.h"
#include "hal/include/lib.h"
#include "hal/include/dev.h"
#include "hal/include/mp.h"


struct armv7_cpu {
    int mp_seq;
    ulong mp_id;
    int cpu_id, cluster_id;
    struct armv7_cpu *next;
};


/*
 * Topology
 */
static int num_cpus = 0;
static struct armv7_cpu *cpus = NULL;

static void make_boot_cpu_mp_seq_zero()
{
    struct armv7_cpu *boot_cpu = NULL;
    ulong boot_mp_id = arch_get_cur_mp_id();

    for (struct armv7_cpu *cpu = cpus; cpu; cpu = cpu->next) {
        if (cpu->mp_id == boot_mp_id) {
            if (!cpu->mp_seq) {
                return;
            }

            boot_cpu = cpu;
            break;
        }
    }

    for (struct armv7_cpu *cpu = cpus; cpu; cpu = cpu->next) {
        if (!cpu->mp_seq) {
            cpu->mp_seq = boot_cpu->mp_seq;
            boot_cpu->mp_seq = 0;
            return;
        }
    }

    panic("Inconsistent MP seq and ID!\n");
}

static void detect_topology(struct driver_param *param)
{
    make_boot_cpu_mp_seq_zero();

    set_num_cpus(num_cpus);

    for (struct armv7_cpu *cpu = cpus; cpu; cpu = cpu->next) {
        set_mp_trans(cpu->mp_seq, cpu->mp_id);
    }

    final_mp_trans();

//     // FIXME: deprecated
//     setup_mp_id_trans(num_cpus, 3,
//                       16, 8,    /* Aff2 */
//                        8, 8,    /* Aff1 - Cluster ID */
//                        0, 8     /* Aff0 - CPU ID */
//                      );
//
//     for (struct armv7_cpu *cpu = cpus; cpu; cpu = cpu->next) {
//         add_mp_id_trans(cpu->mp_id);
//     }
//
//     final_mp_id_trans();
}


/*
 * Driver interface
 */
static void setup(struct driver_param *param)
{
    struct armv7_cpu *record = param->record;

    struct mp_affinity_reg mpr;
    read_cpu_id(mpr.value);

    if (record->mp_id == mpr.lo24) {
        REG_SPECIAL_DRV_FUNC(detect_topology, param, detect_topology);
    }
}

static int probe(struct fw_dev_info *fw_info, struct driver_param *param)
{
    static const char *devtree_names[] = {
        "arm,cortex-a7",
        NULL
    };

    if (fw_info->devtree_node &&
        match_devtree_compatibles(fw_info->devtree_node, devtree_names)
    ) {
        struct armv7_cpu *record = mempool_alloc(sizeof(struct armv7_cpu));
        memzero(record, sizeof(struct armv7_cpu));
        param->record = record;

        struct devtree_prop *prop = devtree_find_prop(fw_info->devtree_node, "reg");
        panic_if(!prop, "Bad ARMv7 CPU devtree node!\n");

        struct mp_affinity_reg mpr;
        mpr.value = devtree_get_prop_data_u32(prop);

        record->mp_seq = num_cpus;
        record->mp_id = mpr.lo24;
        record->cpu_id = mpr.cpu_id;
        record->cluster_id = mpr.cluster_id;

        record->next = cpus;
        cpus = record;
        num_cpus++;

        kprintf("Found ARMv7 CPU @ %ld in cluster %ld\n",
                record->cpu_id, record->cluster_id);
        return FW_DEV_PROBE_OK;
    }

    return FW_DEV_PROBE_FAILED;
}


DECLARE_DEV_DRIVER(armv7_cpu) = {
    .name = "ARMv7 CPU",
    .probe = probe,
    .setup = setup,
};
