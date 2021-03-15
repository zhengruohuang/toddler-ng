#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/msr.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/lib.h"
#include "hal/include/mem.h"
#include "hal/include/int.h"
#include "hal/include/dev.h"


#define MAX_NUM_INT_SRCS 64

struct plic_intc_record {
    int num_devs;
    u64 valid_bitmap;

    ulong priority_base;
    ulong pending_base;
    ulong enable_base;
    ulong threshold;
    ulong complete;
};


static inline u32 plic_read(ulong base, ulong idx)
{
    ulong addr = base + idx * 4;
    return mmio_read32(addr);
}

static inline void plic_write(ulong base, ulong idx, u32 val)
{
    ulong addr = base + idx * 4;
    mmio_write32(addr, val);
}


/*
 * Int manipulation
 */
static void enable_irq(struct driver_param *param, struct int_context *ictxt, int irq_seq)
{
    struct plic_intc_record *record = param->record;

    int reg_num = irq_seq / 32;
    int reg_pos = irq_seq % 32;

    u32 mask = plic_read(record->enable_base, reg_num);
    mask |= 0x1 << reg_pos;
    plic_write(record->enable_base, reg_num, mask);
}

static void disable_irq(struct driver_param *param, struct int_context *ictxt, int irq_seq)
{
    struct plic_intc_record *record = param->record;

    int reg_num = irq_seq / 32;
    int reg_pos = irq_seq % 32;

    u32 mask = plic_read(record->enable_base, reg_num);
    mask &= ~(0x1 << reg_pos);
    plic_write(record->enable_base, reg_num, mask);
}

static void end_irq(struct driver_param *param, struct int_context *ictxt, int irq_seq)
{
    struct plic_intc_record *record = param->record;
    plic_write(record->complete, 0, irq_seq);

    enable_irq(param, ictxt, irq_seq);
}

static void setup_irq(struct driver_param *param, int irq_seq)
{
    struct plic_intc_record *record = param->record;
    record->valid_bitmap |= 0x1ull << irq_seq;

    plic_write(record->priority_base, irq_seq, 1);

    enable_irq(param, NULL, irq_seq);
}

static void disable_irq_all(struct driver_param *param)
{
    struct plic_intc_record *record = param->record;

    for (int cnt = 0, i = 0; cnt < record->num_devs; cnt += 32, i++) {
        plic_write(record->enable_base, i, 0);
    }

    for (int i = 0; i < record->num_devs; i++) {
        plic_write(record->priority_base, i, 0);
    }
}


/*
 * Driver interface
 */
static int pending_irq(struct driver_param *param, struct int_context *ictxt)
{
    struct plic_intc_record *record = param->record;
    u32 claim_id = plic_read(record->complete, 0);
    return claim_id ? claim_id : -1;
}

static void setup(struct driver_param *param)
{
    struct plic_intc_record *record = param->record;
    plic_write(record->threshold, 0, 0);

    disable_irq_all(param);
}

static inline ulong _get_access_window(paddr_t mmio_base_paddr, paddr_t offset, ulong size)
{
    paddr_t paddr = mmio_base_paddr + offset;
    ulong vaddr = get_dev_access_window(paddr, size, DEV_PFN_UNCACHED);
    return vaddr;
}

#define PLIC_CTXT_ID 1

static void *create(struct fw_dev_info *fw_info, struct driver_param *param)
{
    struct plic_intc_record *record = mempool_alloc(sizeof(struct plic_intc_record));
    memzero(record, sizeof(struct plic_intc_record));

    // #int-cells
    int num_int_cells = devtree_get_num_int_cells(fw_info->devtree_node);
    panic_if(num_int_cells != 1, "#int-cells must be 1\n");

    // ndev
    struct devtree_prop *ndev_prop = devtree_find_prop(fw_info->devtree_node, "riscv,ndev");
    record->num_devs = ndev_prop ? devtree_get_prop_data_u32(ndev_prop) : MAX_NUM_INT_SRCS;
    panic_if(record->num_devs > MAX_NUM_INT_SRCS, "PLIC ndev exceeding max supported!\n");

    // reg
    u64 reg = 0, size = 0;
    int next = devtree_get_translated_reg(fw_info->devtree_node, 0, &reg, &size);
    panic_if(next, "riscv,plic0 requires only one reg field!");

    paddr_t mmio_paddr = cast_u64_to_paddr(reg);
    record->priority_base = _get_access_window(mmio_paddr, 0x000000, MAX_NUM_INT_SRCS * 4);
    record->pending_base =  _get_access_window(mmio_paddr, 0x001000, MAX_NUM_INT_SRCS * 4 / 32);
    record->enable_base =   _get_access_window(mmio_paddr, 0x002000 + PLIC_CTXT_ID * 0x80, MAX_NUM_INT_SRCS * 4 / 32);
    record->threshold =     _get_access_window(mmio_paddr, 0x200000 + PLIC_CTXT_ID * 0x1000, 4);
    record->complete =      _get_access_window(mmio_paddr, 0x200004 + PLIC_CTXT_ID * 0x1000, 4);

    kprintf("Found RISC-V PLIC intc\n");
    return record;
}

static const char *plic_intc_devtree_names[] = {
    "riscv,plic0",
    "sifive,plic-1.0.0",
    NULL
};

DECLARE_INTC_DRIVER(plic_intc, "RISC-V Platform-Level Interrupt Controller",
                    plic_intc_devtree_names,
                    create, setup, /*start*/NULL,
                    /*start_cpu*/NULL, /*cpu_power_on*/NULL, /*raw_to_seq*/NULL,
                    setup_irq, enable_irq, disable_irq, end_irq,
                    pending_irq, MAX_NUM_INT_SRCS,
                    INT_SEQ_ALLOC_START);
