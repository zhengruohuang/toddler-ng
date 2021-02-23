#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/lib.h"
#include "hal/include/dev.h"
#include "hal/include/mem.h"


struct arm_pl011_mmio {
    // 0x0
    union {
        u16 value;
        struct {
            u16 reserved    : 4;
            u16 overrun_err : 1;
            u16 break_err   : 1;
            u16 parity_err  : 1;
            u16 frame_err   : 1;
            u16 data        : 8;
        };
    } data;
    u8 zero1[2];

    // 0x4
    union {
        u8 recv_status;
        u8 err_clear;
    };
    u8 zero2[3];

    // 0x8, 0xc, 0x10, 0x14
    u32 reserved1[4];

    // 0x18
    union {
        u16 value;
        struct {
            u16 reserved        : 7;
            u16 ring            : 1;
            u16 tx_fifo_empty   : 1;
            u16 fx_fifl_full    : 1;
            u16 tx_fifo_full    : 1;
            u16 rx_fifo_empty   : 1;
            u16 busy            : 1;
            u16 data_carrier_detect : 1;
            u16 data_set_ready  : 1;
            u16 clear_to_send   : 1;
        };
    } flag;

    // 0x1c
    u32 reserved2;

    // 0x20 - 0xfff
    u32 others[1016];
} packed_struct;

struct arm_pl011_record {
    volatile struct arm_pl011_mmio *mmio;
};


/*
 * putchar
 */
static volatile struct arm_pl011_mmio *pl011_mmio = NULL;

static int pl011_putchar(int ch)
{
    do {
        atomic_mb();
    } while (pl011_mmio->flag.tx_fifo_full);

    atomic_mb();
    pl011_mmio->data.data = ch;
    atomic_mb();

    return 1;
}


// /*
//  * Interrupt
//  */
// static int handler(struct int_context *ictxt, struct kernel_dispatch *kdi)
// {
//     panic("Not implemented!\n");
//     return INT_HANDLE_CALL_KERNEL;
// }


/*
 * Driver interface
 */
static void start(struct driver_param *param)
{
}

static void setup(struct driver_param *param)
{
    struct arm_pl011_record *record = param->record;
    pl011_mmio = record->mmio;

    init_libk_putchar(pl011_putchar);
    kprintf("Putchar switched to PL011 driver\n");
}

static int probe(struct fw_dev_info *fw_info, struct driver_param *param)
{
    static const char *devtree_names[] = {
        "arm,pl011",
        NULL
    };

    if (fw_info->devtree_node &&
        match_devtree_compatibles(fw_info->devtree_node, devtree_names)
    ) {
        struct arm_pl011_record *record = mempool_alloc(sizeof(struct arm_pl011_record));
        memzero(record, sizeof(struct arm_pl011_record));
        param->record = record;
        //param->int_seq = alloc_int_seq(handler);

        u64 reg = 0, size = 0;
        int next = devtree_get_translated_reg(fw_info->devtree_node, 0, &reg, &size);
        panic_if(next, "arm,pl011 only supports one reg field!");

        paddr_t mmio_paddr = cast_u64_to_paddr(reg);
        ulong mmio_size = cast_paddr_to_vaddr(size);
        ulong mmio_vaddr = get_dev_access_window(mmio_paddr, mmio_size, DEV_PFN_UNCACHED);

        record->mmio = (void *)mmio_vaddr;

        kprintf("Found ARM PrimeCell UART PL011 @ %lx, window @ %lx\n",
                mmio_paddr, mmio_vaddr);
        return FW_DEV_PROBE_OK;
    }

    return FW_DEV_PROBE_FAILED;
}

DECLARE_DEV_DRIVER(arm_pl011) = {
    .name = "ARM PrimeCell UART PL011",
    .probe = probe,
    .setup = setup,
    .start = start,
};
