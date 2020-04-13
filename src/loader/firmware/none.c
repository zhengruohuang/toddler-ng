#include "common/include/inttypes.h"
#include "loader/include/firmware.h"


DECLARE_FIRMWARE_DRIVER(none) = {
    .name = "none",
    .need_fdt = 1,
};
