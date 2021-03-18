#include "common/include/inttypes.h"
#include "common/include/msr.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/hal.h"
#include "hal/include/lib.h"
#include "hal/include/dev.h"
#include "hal/include/mp.h"


struct or1200_cpu {
    ulong mp_id;
    struct or1200_cpu *next;
};


/*
 * Topology
 */
static struct or1200_cpu *cpus = NULL;

static void detect_topology(struct driver_param *param)
{
    ulong num_cores = 0;
    read_num_cores(num_cores);

    set_num_cpus(num_cores);

    for (ulong i = 0; i < num_cores; i++) {
        set_mp_trans(i, i);
    }

    final_mp_trans();
}


/*
 * Driver interface
 */
static void setup(struct driver_param *param)
{
    struct or1200_cpu *record = param->record;

    ulong mp_id = arch_get_cur_mp_id();
    if (record->mp_id == mp_id) {
        REG_SPECIAL_DRV_FUNC(detect_topology, param, detect_topology);
    }
}

static int probe(struct fw_dev_info *fw_info, struct driver_param *param)
{
    static const char *devtree_names[] = {
        "opencores,or1200",
        NULL
    };

    if (fw_info->devtree_node &&
        match_devtree_compatibles(fw_info->devtree_node, devtree_names)
    ) {
        struct or1200_cpu *record = mempool_alloc(sizeof(struct or1200_cpu));
        memzero(record, sizeof(struct or1200_cpu));
        param->record = record;

        struct devtree_prop *prop = devtree_find_prop(fw_info->devtree_node, "reg");
        panic_if(!prop, "Bad OR1200 CPU devtree node!\n");

        record->mp_id = devtree_get_prop_data_u32(prop);
        record->next = cpus;
        cpus = record;

        kprintf("Found OR1200 CPU\n");
        return FW_DEV_PROBE_OK;
    }

    return FW_DEV_PROBE_FAILED;
}


DECLARE_DEV_DRIVER(or1200_cpu) = {
    .name = "OR1200 CPU",
    .probe = probe,
    .setup = setup,
};
