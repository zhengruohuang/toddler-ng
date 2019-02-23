#include "common/include/inttypes.h"
#include "loader/include/lib.h"
#include "loader/include/periph.h"


#define REG_DRV(comp, init)     { comp, init }

struct dev_driver {
    const char *compatible;
    int (*init)(const void *data, const char *opts);
};

static struct dev_driver drivers[] = {
    REG_DRV("simple-graphic-fb", NULL),
    REG_DRV("simple-text-fb", NULL),
    REG_DRV("ns16550", init_ns16550),

    // The end of driver list
    REG_DRV(NULL, NULL),
};


int create_periph(const char *dev_name, const void *data, const char *opts)
{
    for (struct dev_driver *drv = drivers; drv && drv->compatible; drv++) {
        if (!strcmp(dev_name, drv->compatible)) {
            return drv->init(data, opts);
        }
    }

    return -1;
}
