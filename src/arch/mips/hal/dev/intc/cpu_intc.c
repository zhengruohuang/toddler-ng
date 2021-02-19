#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/msr.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/lib.h"
#include "hal/include/mem.h"
#include "hal/include/int.h"
#include "hal/include/dev.h"


#define MAX_NUM_INT_SRCS    8

struct mips_cpu_intc_record {
    int last_seq;
    struct driver_param *int_devs[MAX_NUM_INT_SRCS];
};


/*
 * Int manipulation
 */
static void enable_irq(struct mips_cpu_intc_record *record, int seq)
{
    if (seq >= MAX_NUM_INT_SRCS) {
        panic("Unknown IRQ seq: %d\n", seq);
        return;
    }

    struct cp0_status sr;
    read_cp0_status(sr.value);
    sr.im |= 0x1ul << seq;
    write_cp0_status(sr.value);
}

static void disable_irq(struct mips_cpu_intc_record *record, int seq)
{
    if (seq >= MAX_NUM_INT_SRCS) {
        panic("Unknown IRQ seq: %d\n", seq);
        return;
    }

    struct cp0_status sr;
    read_cp0_status(sr.value);
    sr.im &= ~(0x1ul << seq);
    write_cp0_status(sr.value);
}

static void disable_all(struct mips_cpu_intc_record *record)
{
    struct cp0_status sr;
    read_cp0_status(sr.value);
    sr.im = 0;
    write_cp0_status(sr.value);
}


/*
 * Interrupt handler
 */
static int handle(struct int_context *ictxt, struct kernel_dispatch *kdi,
                  struct mips_cpu_intc_record *record, int seq)
{
    disable_irq(record, seq);
    struct driver_param *int_dev = record->int_devs[seq];

    int handle_type = INT_HANDLE_SIMPLE;
    if (int_dev && int_dev->int_seq) {
        ictxt->param = int_dev->record;
        handle_type = invoke_int_handler(int_dev->int_seq, ictxt, kdi);

        if (INT_HANDLE_KEEP_MASKED & ~handle_type) {
            enable_irq(record, seq);
        }
    } else if (int_dev) {
        // TODO: need a better way
        kdi->param0 = int_dev->user_seq;
        handle_type = INT_HANDLE_CALL_KERNEL | INT_HANDLE_KEEP_MASKED;
    }

    return handle_type & INT_HANDLE_CALL_KERNEL ?
            INT_HANDLE_CALL_KERNEL : INT_HANDLE_SIMPLE;
}

static int handler(struct int_context *ictxt, struct kernel_dispatch *kdi)
{
    struct mips_cpu_intc_record *record = ictxt->param;

    struct cp0_cause cause;
    read_cp0_cause(cause.value);

    int seq = record->last_seq + 1;
    if (seq >= MAX_NUM_INT_SRCS) {
        seq = 0;
    }

    for (int i = 0; i < MAX_NUM_INT_SRCS; i++) {
        if (cause.ip & (0x1ul << seq) && record->int_devs[seq]) {
            return handle(ictxt, kdi, record, seq);
        }

        if (++seq >= MAX_NUM_INT_SRCS) {
            seq = 0;
        }
    }

    return INT_HANDLE_SIMPLE;
}


/*
 * EOI
 */
static void eoi(struct driver_param *param, struct driver_int_encode *encode, struct driver_param *dev)
{
    struct mips_cpu_intc_record *record = param->record;
    int num_ints = encode->size / sizeof(int);
    int *int_srcs = encode->data;

    for (int g = 0; g < num_ints; g++) {
        int seq = swap_big_endian32(int_srcs[g]);
        enable_irq(record, seq);
    }
}


/*
 * Driver interface
 */
static void start(struct driver_param *param)
{

}

static void setup_int(struct driver_param *param, struct driver_int_encode *encode, struct driver_param *dev)
{
    kprintf("set int, data: %p, size: %d\n", encode->data, encode->size);

    if (!encode->size || !encode->data) {
        return;
    }

    //kprintf("set int, data: %p, size: %d\n", encode->data, encode->size);

    struct mips_cpu_intc_record *record = param->record;
    int num_ints = encode->size / sizeof(int);
    int *int_srcs = encode->data;

    for (int g = 0; g < num_ints; g++) {
        int seq = swap_big_endian32(int_srcs[g]);

        kprintf("to enable seq: %d\n", seq);

        enable_irq(record, seq);
        record->int_devs[seq] = dev;
    }
}

static void setup(struct driver_param *param)
{
    struct mips_cpu_intc_record *record = param->record;
    disable_all(record);
}

static int probe(struct fw_dev_info *fw_info, struct driver_param *param)
{
    static const char *devtree_names[] = {
        "mti,cpu-interrupt-controller",
        "mips,cpu-cp0-cause",
        NULL
    };

    if (fw_info->devtree_node &&
        match_devtree_compatibles(fw_info->devtree_node, devtree_names)
    ) {
        struct mips_cpu_intc_record *record = mempool_alloc(sizeof(struct mips_cpu_intc_record));
        memzero(record, sizeof(struct mips_cpu_intc_record));
        param->record = record;
        param->int_seq = alloc_int_seq(handler);

        int num_int_cells = devtree_get_num_int_cells(fw_info->devtree_node);
        panic_if(num_int_cells != 1, "#int-cells must be 1\n");

        kprintf("Found MIPS CP0 top-level intc\n");
        return FW_DEV_PROBE_OK;
    }

    return FW_DEV_PROBE_FAILED;
}


DECLARE_DEV_DRIVER(mips_cpu_intc) = {
    .name = "MIPS CP0 Interrupt Controller",
    .probe = probe,
    .setup = setup,
    .setup_int = setup_int,
    .start = start,
    .eoi = eoi,
};
