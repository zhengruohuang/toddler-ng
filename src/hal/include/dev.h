#ifndef __HAL_INCLUDE_DEV__
#define __HAL_INCLUDE_DEV__


#include "common/include/devtree.h"
#include "hal/include/int.h"


/*
 * Device driver
 */
#include "hal/include/driver.h"

enum fw_dev_probe_result {
    FW_DEV_PROBE_FAILED,
    FW_DEV_PROBE_OK,
};

extern int match_devtree_compatible(struct devtree_node *node, const char *drv);
extern int match_devtree_compatibles(struct devtree_node *node, const char *drvs[]);

extern void start_all_devices();
extern void start_all_devices_mp();

extern int handle_dev_int(struct int_context *ictxt, struct kernel_dispatch *kdi);

extern void *user_int_register(ulong phandle, ulong user_seq);
extern void user_int_eoi(void *hal_dev);

extern void init_dev();


/*
 * Special
 */
typedef void (*start_cpu_t)(struct driver_param *param, int seq, ulong id, ulong entry);
typedef void (*detect_topology_t)(struct driver_param *param);
typedef void (*cpu_power_on_t)(struct driver_param *param, int seq, ulong id);
typedef void (*cpu_power_off_t)(struct driver_param *param, int seq, ulong id);

struct special_drv_func_record {
    void *param;
    union {
        void (*func)();
        start_cpu_t start_cpu;
        detect_topology_t detect_topology;
        cpu_power_on_t on_cpu_power_on;
        cpu_power_off_t on_cpu_power_off;
    };
};

struct special_drv_func_list {
    struct special_drv_func_list *next;
    struct special_drv_func_record record;
};

struct special_drv_funcs {
    struct special_drv_func_record start_cpu;
    struct special_drv_func_record detect_topology;

    struct special_drv_func_list *cpu_power_on;
    struct special_drv_func_list *cpu_power_off;
};

#define REG_SPECIAL_DRV_FUNC(type, param, func)             \
    extern void reg_##type##_t(void *p, type##_t f);        \
    reg_##type##_t(param, func)

#define DECLARE_REG_SPECIAL_DRV_FUNC(type, param, func)     \
    reg_##type##_t(void *param, type##_t func)

enum drv_func_invoke_result {
    DRV_FUNC_INVOKE_OK = 0,
    DRV_FUNC_INVOKE_FAILED,
    DRV_FUNC_INVOKE_NOT_REG,
};

extern int drv_func_start_cpu(int seq, ulong id, ulong entry);
extern int drv_func_detect_topology();
extern int drv_func_cpu_power_on(int seq, ulong id);
extern int drv_func_cpu_power_off(int seq, ulong id);


/*
 * Device access window
 */
enum dev_pfn_cache_query_type {
    DEV_PFN_UNCACHED    = 0,
    DEV_PFN_CACHED      = 1,
    DEV_PFN_ANY_CACHED  = 2,
};

extern ulong get_dev_access_window(paddr_t paddr, ulong size, int cached);


/*
 * Clock
 */
enum clock_quality {
    CLOCK_QUALITY_NO_CLOCK,
    CLOCK_QUALITY_UNRELIABLE,
    CLOCK_QUALITY_PERIODIC_LOW_RES,
    CLOCK_QUALITY_PERIODIC_HIGH_RES,
    CLOCK_QUALITY_HIGH_QUALITY,
};

extern int clock_source_register(int quality);

extern u64 clock_get_ms();
extern void clock_set_ms(int clk_idx, u64 new_ms);
extern void clock_advance_ms(int clk_idx, ulong advance);


/*
 * IO
 */
extern u8 mmio_read8(ulong addr);
extern void mmio_write8(ulong addr, u8 val);
extern u16 mmio_read16(ulong addr);
extern void mmio_write16(ulong addr, u16 val);
extern u32 mmio_read32(ulong addr);
extern void mmio_write32(ulong addr, u32 val);

extern u8 port_read8(ulong addr);
extern void port_write8(ulong addr, u8 val);
extern u16 port_read16(ulong addr);
extern void port_write16(ulong addr, u16 val);
extern u32 port_read32(ulong addr);
extern void port_write32(ulong addr, u32 val);

#if (ARCH_WIDTH == 64)
extern u64 mmio_read64(ulong addr);
extern void mmio_write64(ulong addr, u64 val);
extern u64 port_read64(ulong addr);
extern void port_write64(ulong addr, u64 val);
#endif


#endif
