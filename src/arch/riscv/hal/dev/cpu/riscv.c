#include "common/include/inttypes.h"
#include "common/include/msr.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/hal.h"
#include "hal/include/lib.h"
#include "hal/include/dev.h"
#include "hal/include/mp.h"


struct riscv_cpu {
    ulong mp_id;
    int mp_seq;
    void *cpu_fw_node;
    struct riscv_cpu *next;
};


/*
 * Topology
 */
static int num_cpus = 0;
static struct riscv_cpu *cpus = NULL;

static void make_boot_cpu_mp_seq_zero()
{
    struct riscv_cpu *boot_cpu = NULL;
    ulong boot_mp_id = arch_get_cur_mp_id();

    for (struct riscv_cpu *cpu = cpus; cpu; cpu = cpu->next) {
        if (cpu->mp_id == boot_mp_id) {
            if (!cpu->mp_seq) {
                return;
            }

            boot_cpu = cpu;
            break;
        }
    }

    for (struct riscv_cpu *cpu = cpus; cpu; cpu = cpu->next) {
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

    for (struct riscv_cpu *cpu = cpus; cpu; cpu = cpu->next) {
        set_mp_trans(cpu->mp_seq, cpu->mp_id);
    }

    final_mp_trans();
}

static void detect_cpu_local_intc(struct driver_param *param)
{
    struct riscv_cpu *record = param->record;
    kprintf("Detecting CPU local intc, mp id: %lx, seq: %d\n",
            record->mp_id, record->mp_seq);

    set_cpu_local_intc(record->cpu_fw_node, record->mp_seq);
}


/*
 * Driver interface
 */
static void setup(struct driver_param *param)
{
    struct riscv_cpu *record = param->record;

    ulong mp_id = arch_get_cur_mp_id();
    if (record->mp_id == mp_id) {
        REG_SPECIAL_DRV_FUNC(detect_topology, param, detect_topology);
    }

    REG_SPECIAL_DRV_FUNC(detect_cpu_local_intc, param, detect_cpu_local_intc);
}

static int probe(struct fw_dev_info *fw_info, struct driver_param *param)
{
    static const char *devtree_names[] = {
        "riscv",
        NULL
    };

    if (fw_info->devtree_node &&
        match_devtree_compatibles(fw_info->devtree_node, devtree_names)
    ) {
        struct riscv_cpu *record = mempool_alloc(sizeof(struct riscv_cpu));
        memzero(record, sizeof(struct riscv_cpu));
        param->record = record;

        struct devtree_prop *prop = devtree_find_prop(fw_info->devtree_node, "reg");
        panic_if(!prop, "Bad RISC-V CPU devtree node!\n");

        record->cpu_fw_node = fw_info->devtree_node;
        record->mp_seq = num_cpus;
        record->mp_id = devtree_get_prop_data_u32(prop);
        record->next = cpus;
        cpus = record;
        num_cpus++;

        kprintf("Found RISC-V CPU, Hart @ %ld\n", record->mp_id);
        return FW_DEV_PROBE_OK;
    }

    return FW_DEV_PROBE_FAILED;
}


DECLARE_DEV_DRIVER(riscv_cpu) = {
    .name = "RISC-V CPU",
    .probe = probe,
    .setup = setup,
};
