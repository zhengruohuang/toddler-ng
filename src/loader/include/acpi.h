#ifndef __LOADER_INCLUDE_ACPI_H__
#define __LOADER_INCLUDE_ACPI_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


extern void init_acpi(void *rsdp_ptr);
extern void *find_acpi_rsdp(ulong start, ulong end);


#endif
