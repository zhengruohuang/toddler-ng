#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "hal/include/vecnum.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/lib.h"
#include "hal/include/mem.h"
#include "hal/include/int.h"
#include "hal/include/dev.h"
#include "hal/include/sbi.h"


#define INT_FREQ_DIV    10
#define MS_PER_INT      (1000 / (INT_FREQ_DIV))


struct clint_timer_record {
    ulong freq;
    u32 timer_step;
    ulong mtime_base;
};


/*
 * IO helpers
 */
static inline u64 clint_mtime_read(struct clint_timer_record *record)
{
    ulong addr = record->mtime_base;
    u64 mtime = 0;
#if (ARCH_WIDTH == 32)
    u32 upper = mmio_read32(addr + 4ul);
    u32 lower = mmio_read32(addr);
    while (upper != mmio_read32(addr + 4ul)) {
        atomic_mb();
        upper = mmio_read32(addr + 4ul);
        lower = mmio_read32(addr);
    }
    mtime = ((u64)upper << 32) | ((u64)lower);
#elif (ARCH_WIDTH == 64)
    mtime = mmio_read64(addr);
#endif
    return mtime;
}


/*
 * Update compare
 */
static void update_compare(struct clint_timer_record *record)
{
    u64 mtimecmp = clint_mtime_read(record);
    mtimecmp += record->timer_step;
    sbi_set_timer(mtimecmp);
}



/*
 * Interrupt
 */
static int clint_timer_int_handler(struct driver_param *param, struct int_context *ictxt)
{
    struct clint_timer_record *record = param->record;
    update_compare(record);

    return INT_HANDLE_CALL_KERNEL;
}


/*
 * Driver interface
 */
static void cpu_power_on(struct driver_param *param, int seq, ulong id)
{
    struct clint_timer_record *record = param->record;
    update_compare(record);

    kprintf("MP Timer started! record @ %p, step: %d\n", record, record->timer_step);
}

static void start(struct driver_param *param)
{
    struct clint_timer_record *record = param->record;
    update_compare(record);

    kprintf("Timer started! record @ %p, step: %d\n", record, record->timer_step);
}

static void setup(struct driver_param *param)
{
    struct clint_timer_record *record = param->record;

    // Set up the actual freq
    record->timer_step = record->freq / INT_FREQ_DIV;
    kprintf("Timer freq @ %luHz, step set @ %u, record @ %p\n", record->timer_step, record);
}

static void *create(struct fw_dev_info *fw_info, struct driver_param *param)
{
    struct clint_timer_record *record = mempool_alloc(sizeof(struct clint_timer_record));
    memzero(record, sizeof(struct clint_timer_record));

    u64 reg = 0, size = 0;
    int next = devtree_get_translated_reg(fw_info->devtree_node, 0, &reg, &size);
    panic_if(next, "riscv,clint0 requires only one reg field!");

    paddr_t mmio_paddr = cast_u64_to_paddr(reg);
    paddr_t mtime_paddr = mmio_paddr + 0xbff8ull;
    ulong mmio_vaddr = get_dev_access_window(mtime_paddr, 8, DEV_PFN_UNCACHED);
    record->mtime_base = mmio_vaddr;

    record->freq = 0x989680;    // TODO: read devtree

    kprintf("Found RISC-V clint timer!\n");
    return record;
}

static const char *clint_timer_devtree_names[] = {
    "riscv,clint0",
    NULL
};

DECLARE_TIMER_DRIVER(clint_timer, "RISC-V Clint Timer",
                     clint_timer_devtree_names,
                     create, setup, start, cpu_power_on,
                     CLOCK_QUALITY_PERIODIC_LOW_RES, MS_PER_INT,
                     INT_SEQ_ALLOC_START, clint_timer_int_handler);
