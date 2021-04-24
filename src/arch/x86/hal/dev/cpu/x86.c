#include "common/include/inttypes.h"
#include "common/include/msr.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/hal.h"
#include "hal/include/lib.h"
#include "hal/include/dev.h"
#include "hal/include/mp.h"


struct x86_cpu {
    ulong mp_id;
    int mp_seq;
    void *cpu_fw_node;
    struct x86_cpu *next;

    int count_in_group;
    struct x86_cpu *group_lead;
};


/*
 * Topology
 */
static int num_cpus = 0;
static struct x86_cpu *cpus = NULL;

static void make_boot_cpu_mp_seq_zero()
{
    struct x86_cpu *boot_cpu = NULL;
    ulong boot_mp_id = arch_get_cur_mp_id();

    for (struct x86_cpu *cpu = cpus; cpu; cpu = cpu->next) {
        if (cpu->mp_id == boot_mp_id) {
            if (!cpu->mp_seq) {
                return;
            }

            boot_cpu = cpu;
            break;
        }
    }

    for (struct x86_cpu *cpu = cpus; cpu; cpu = cpu->next) {
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

    for (struct x86_cpu *cpu = cpus; cpu; cpu = cpu->next) {
        set_mp_trans(cpu->mp_seq, cpu->mp_id);
    }

    final_mp_trans();
}


/*
 * Driver interface
 */
static void setup(struct driver_param *param)
{
    struct x86_cpu *record = param->record;

    //ulong mp_id = arch_get_cur_mp_id();
    //if (record->mp_id == mp_id) {
        REG_SPECIAL_DRV_FUNC(detect_topology, param, detect_topology);
    //}
}

static int probe(struct fw_dev_info *fw_info, struct driver_param *param)
{
    static const char *devtree_names[] = {
        "x86", "ia32", "amd64",
        NULL
    };

    if (fw_info->devtree_node &&
        match_devtree_compatibles(fw_info->devtree_node, devtree_names)
    ) {
        int count = 1;
        struct devtree_prop *prop_count = devtree_find_prop(fw_info->devtree_node, "acpi,cpu-count");
        if (prop_count) {
            count = devtree_get_prop_data_u32(prop_count);
        }

        int size = sizeof(struct x86_cpu) * count;
        struct x86_cpu *records = mempool_alloc(size);
        memzero(records, size);
        param->record = records;

        if (!prop_count) {
            struct devtree_prop *prop = devtree_find_prop(fw_info->devtree_node, "reg");
            panic_if(!prop, "Bad x86 CPU devtree node!\n");

            struct x86_cpu *record = records;
            record->cpu_fw_node = fw_info->devtree_node;
            record->mp_seq = num_cpus;
            record->mp_id = devtree_get_prop_data_u32(prop);
            record->next = cpus;
            record->count_in_group = 1;
            record->group_lead = record;
            cpus = record;
            num_cpus++;

            kprintf("Found x86 CPU, APIC ID @ %lx\n", record->mp_id);
        }

        else {
            struct devtree_prop *prop_cfg = devtree_find_prop(fw_info->devtree_node, "acpi,cpu-config");
            u32 *cfg = devtree_get_prop_data(prop_cfg);

            for (int i = 0, offset = 0; i < count; i++, offset += 2) {
                u32 acpi_proc_id = swap_big_endian32(cfg[offset]);
                u32 apic_id = swap_big_endian32(cfg[offset + 1]);

                struct x86_cpu *record = &records[i];
                record->cpu_fw_node = fw_info->devtree_node;
                record->mp_seq = num_cpus;
                record->mp_id = apic_id;
                record->next = cpus;
                record->count_in_group = count;
                record->group_lead = records;
                cpus = record;
                num_cpus++;

                kprintf("Found x86 CPU, Proc ID @ %x APIC ID @ %x\n",
                        acpi_proc_id, apic_id);
            }
        }

        return FW_DEV_PROBE_OK;
    }

    return FW_DEV_PROBE_FAILED;
}


DECLARE_DEV_DRIVER(x86_cpu) = {
    .name = "x86 CPU",
    .probe = probe,
    .setup = setup,
};
