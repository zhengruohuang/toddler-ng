#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "common/include/msr.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/lib.h"
#include "hal/include/mem.h"
#include "hal/include/int.h"
#include "hal/include/dev.h"


#define MAX_NUM_INT_SRCS 32

struct or1k_intc_record {
    int last_seq;
    struct driver_param *int_devs[MAX_NUM_INT_SRCS];
};


/*
 * Int manipulation
 */
static void eoi_irq(struct or1k_intc_record *record, int seq)
{
    if (seq >= MAX_NUM_INT_SRCS) {
        panic("Unknown IRQ seq: %d\n", seq);
        return;
    }

    u32 bitmap = 0x1 << seq;
    write_pic_status(bitmap);

    read_pic_mask(bitmap);
    bitmap |= 0x1 << seq;
    write_pic_mask(bitmap);
}

static void enable_irq(struct or1k_intc_record *record, int seq)
{
    if (seq >= MAX_NUM_INT_SRCS) {
        panic("Unknown IRQ seq: %d\n", seq);
        return;
    }

    u32 bitmap;
    read_pic_mask(bitmap);
    bitmap |= 0x1 << seq;
    write_pic_mask(bitmap);
}

static void disable_irq(struct or1k_intc_record *record, int seq)
{
    if (seq >= MAX_NUM_INT_SRCS) {
        panic("Unknown IRQ seq: %d\n", seq);
        return;
    }

    u32 bitmap = 0;

    read_pic_mask(bitmap);
    bitmap &= ~(0x1 << seq);
    write_pic_mask(bitmap);

    bitmap = 0x1 << seq;
    write_pic_status(bitmap);
}

static void disable_all(struct or1k_intc_record *record)
{
    write_pic_mask(0);
}


/*
 * Interrupt handler
 */
static int handle(struct int_context *ictxt, struct kernel_dispatch *kdi,
                  struct or1k_intc_record *record, int seq)
{
    disable_irq(record, seq);

    // FIXME: for some strange reason, there's a suprious interrupt even after
    // disabling the IRQ, writing something to UART seems to fix this issue
    kprintf("%c", 24);

    struct driver_param *int_dev = record->int_devs[seq];

    int handle_type = INT_HANDLE_SIMPLE;
    if (int_dev && int_dev->int_seq) {
        ictxt->param = int_dev->record;
        handle_type = invoke_int_handler(int_dev->int_seq, ictxt, kdi);

        if (INT_HANDLE_KEEP_MASKED & ~handle_type) {
            eoi_irq(record, seq);
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
    struct or1k_intc_record *record = ictxt->param;

    u32 enabled_bitmap, pending_bitmap;
    read_pic_mask(enabled_bitmap);
    read_pic_status(pending_bitmap);

    u32 bitmap = enabled_bitmap & pending_bitmap;

//     if (bitmap)
//     kprintf("Interrupt, enabled: %x, pending: %x, bitmap: %x\n", enabled_bitmap, pending_bitmap, bitmap);

    int seq = record->last_seq + 1;
    if (seq >= MAX_NUM_INT_SRCS) {
        seq = 0;
    }

    for (int i = 0; i < MAX_NUM_INT_SRCS; i++) {
        if (bitmap & (0x1ul << seq) && record->int_devs[seq]) {
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
    struct or1k_intc_record *record = param->record;
    int num_ints = encode->size / sizeof(int);
    int *int_srcs = encode->data;

    for (int g = 0; g < num_ints; g++) {
        int seq = swap_big_endian32(int_srcs[g]);
        eoi_irq(record, seq);
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

    struct or1k_intc_record *record = param->record;
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
    struct or1k_intc_record *record = param->record;
    disable_all(record);
}

static int probe(struct fw_dev_info *fw_info, struct driver_param *param)
{
    static const char *devtree_names[] = {
        "opencores,or1k-pic",
        "or1k-pic",
        NULL
    };

    if (fw_info->devtree_node &&
        match_devtree_compatibles(fw_info->devtree_node, devtree_names)
    ) {
        struct or1k_intc_record *record = mempool_alloc(sizeof(struct or1k_intc_record));
        memzero(record, sizeof(struct or1k_intc_record));
        param->record = record;
        param->int_seq = alloc_int_seq(handler);

        int num_int_cells = devtree_get_num_int_cells(fw_info->devtree_node);
        panic_if(num_int_cells != 1, "#int-cells must be 1\n");

        kprintf("Found OpenCores OR1K CPU top-level intc\n");
        return FW_DEV_PROBE_OK;
    }

    return FW_DEV_PROBE_FAILED;
}


DECLARE_DEV_DRIVER(or1k_intc) = {
    .name = "OpenCores OR1K Interrupt Controller",
    .probe = probe,
    .setup = setup,
    .setup_int = setup_int,
    .start = start,
    .eoi = eoi,
};
