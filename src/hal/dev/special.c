#include "common/include/inttypes.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/dev.h"


static struct special_drv_funcs funcs = { };


/*
 * Register
 */
static void reg_unique(const char *name, struct special_drv_func_record *record,
                       struct driver_param *param, void *func)
{
    panic_if(record->func, "Function already registered: %s\n", name);
    record->param = param;
    record->func = func;
}

static void reg_multi(const char *name, struct special_drv_func_list **list,
                      struct driver_param *param, void *func)
{
    struct special_drv_func_list *node =
            mempool_alloc(sizeof(struct special_drv_func_list));

    node->record.param = param;
    node->record.func = func;

    node->next = *list;
    *list = node;
}

void DECLARE_REG_SPECIAL_DRV_FUNC(start_cpu, param, func)
{
    reg_unique("start_cpu", &funcs.start_cpu, param, func);
}

void DECLARE_REG_SPECIAL_DRV_FUNC(detect_topology, param, func)
{
    reg_unique("detect_topo", &funcs.detect_topology, param, func);
}

void DECLARE_REG_SPECIAL_DRV_FUNC(cpu_power_on, param, func)
{
    reg_multi("cpu_power_on", &funcs.cpu_power_on, param, func);
}

void DECLARE_REG_SPECIAL_DRV_FUNC(cpu_power_off, param, func)
{
    reg_multi("cpu_power_off", &funcs.cpu_power_off, param, func);
}


/*
 * Invoke
 */
int drv_func_start_cpu(int seq, ulong id, ulong entry)
{
    if (funcs.start_cpu.func) {
        funcs.start_cpu.start_cpu(funcs.start_cpu.param, seq, id, entry);
        return DRV_FUNC_INVOKE_OK;
    }

    return DRV_FUNC_INVOKE_NOT_REG;
}

int drv_func_detect_topology()
{
    if (funcs.detect_topology.func) {
        funcs.detect_topology.detect_topology(funcs.detect_topology.param);
        return DRV_FUNC_INVOKE_OK;
    }

    return DRV_FUNC_INVOKE_NOT_REG;
}

int drv_func_cpu_power_on(int seq, ulong id)
{
    for (struct special_drv_func_list *n = funcs.cpu_power_on;
         n; n = n->next
    ) {
        panic_if(!n->record.func, "cpu_power_on invalid!\n");
        n->record.on_cpu_power_on(n->record.param, seq, id);
    }

    return DRV_FUNC_INVOKE_OK;
}

int drv_func_cpu_power_off(int seq, ulong id)
{
    for (struct special_drv_func_list *n = funcs.cpu_power_off;
         n; n = n->next
    ) {
        panic_if(!n->record.func, "cpu_power_off invalid!\n");
        n->record.on_cpu_power_off(n->record.param, seq, id);
    }

    return DRV_FUNC_INVOKE_OK;
}
