#include "common/include/inttypes.h"
#include "common/include/msr.h"
#include "hal/include/kprintf.h"

static volatile int has_pmu = 0;

void pmu_idle()
{
    if (!has_pmu) {
        return;
    }

    struct power_mgmt_reg pmr = { .value = 0 };
    pmr.doze = 1;
    write_pmr(pmr.value);
}

void pmu_halt()
{
    if (!has_pmu) {
        return;
    }

    struct power_mgmt_reg pmr = { .value = 0 };
    pmr.suspend = 1;
    write_pmr(pmr.value);
}

void init_pmu()
{
    struct unit_present_reg upr;
    read_upr(upr.value);

    if (upr.valid && upr.power_mgmt) {
        has_pmu = 1;
    }

    kprintf("Power management unit present: %d\n", has_pmu);
}

void init_pmu_mp()
{
    // TODO: per CPU
}
