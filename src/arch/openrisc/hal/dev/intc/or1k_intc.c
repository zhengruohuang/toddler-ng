#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "common/include/msr.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/vecnum.h"
#include "hal/include/lib.h"
#include "hal/include/mem.h"
#include "hal/include/int.h"
#include "hal/include/dev.h"


// Seq == 32: timer
#define MAX_NUM_INT_SRCS 33

struct or1k_intc_record {
    u32 valid_bitmap;
    int last_seq;
};


/*
 * Int manipulation
 */
static inline void _clear_irq(int seq)
{
    u32 bitmap = 0x1 << seq;
    write_pic_status(bitmap);
}

static inline void _enable_irq(int seq)
{
    u32 bitmap;
    read_pic_mask(bitmap);
    bitmap |= 0x1 << seq;
    write_pic_mask(bitmap);
}

static inline void _disable_irq(int seq)
{
    u32 bitmap;
    read_pic_mask(bitmap);
    bitmap &= ~(0x1 << seq);
    write_pic_mask(bitmap);
}


/*
 * IRQ
 */


static void enable_irq(struct driver_param *param, struct int_context *ictxt, int seq)
{
    if (seq >= 32) {
        // TODO
    } else {
        _enable_irq(seq);
    }
}

static void disable_irq(struct driver_param *param, struct int_context *ictxt, int seq)
{
    if (seq >= 32) {
        // TODO
    } else {
        _disable_irq(seq);
        _clear_irq(seq);

        // FIXME: for some strange reason, there's a suprious interrupt right
        // after disabling the IRQ
        // Writing anything to UART seems to fix this issue
        kprintf("%c", 24);
    }
}

static void end_irq(struct driver_param *param, struct int_context *ictxt, int seq)
{
    enable_irq(param, ictxt, seq);
}

static void setup_irq(struct driver_param *param, int seq)
{
    struct or1k_intc_record *record = param->record;
    record->valid_bitmap |= 0x1ul << seq;

    enable_irq(param, NULL, seq);
}



/*
 * Interrupt handler
 */
static int pending_irq(struct driver_param *param, struct int_context *ictxt)
{
    if (ictxt->error_code == EXCEPT_NUM_TIMER) {
        return 32;
    }

    panic_if(ictxt->error_code != EXCEPT_NUM_INTERRUPT,
             "error_code must be EXCEPT_NUM_INTERRUPT!\n");

    struct or1k_intc_record *record = param->record;

    u32 enabled_bitmap, pending_bitmap;
    read_pic_mask(enabled_bitmap);
    read_pic_status(pending_bitmap);

    u32 bitmap = enabled_bitmap & pending_bitmap & record->valid_bitmap;
//     kprintf("Interrupt, enabled: %x, pending: %x, bitmap: %x\n",
//             enabled_bitmap, pending_bitmap, bitmap);

    int seq = record->last_seq + 1;
    if (seq >= MAX_NUM_INT_SRCS) {
        seq = 0;
    }

    for (int i = 0; i < MAX_NUM_INT_SRCS; i++) {
        if (bitmap & (0x1ul << seq)) {
            return seq;
        }

        if (++seq >= MAX_NUM_INT_SRCS) {
            seq = 0;
        }
    }

    return -1;
}


/*
 * Init
 */
static void disable_all(struct or1k_intc_record *record)
{
    write_pic_mask(0);
}

static void setup(struct driver_param *param)
{
    struct or1k_intc_record *record = param->record;
    disable_all(record);
}

/*
 * Driver interface
 */
static void *create(struct fw_dev_info *fw_info, struct driver_param *param)
{
    struct or1k_intc_record *record = mempool_alloc(sizeof(struct or1k_intc_record));
    memzero(record, sizeof(struct or1k_intc_record));

    int num_int_cells = devtree_get_num_int_cells(fw_info->devtree_node);
    panic_if(num_int_cells != 1, "#int-cells must be 1\n");

    kprintf("Found OpenCores OR1K CPU top-level intc\n");
    return record;
}

static const char *or1k_intc_devtree_names[] = {
    "opencores,or1k-pic",
    "or1k-pic",
    NULL
};

DECLARE_INTC_DRIVER(or1k_intc, "OpenCores OR1K Interrupt Controller",
                    or1k_intc_devtree_names,
                    create, setup, /*start*/NULL,
                    /*start_cpu*/NULL, /*cpu_power_on*/NULL, /*raw_to_seq*/NULL,
                    setup_irq, enable_irq, disable_irq, end_irq,
                    pending_irq, MAX_NUM_INT_SRCS,
                    INT_SEQ_ALLOC_START);
