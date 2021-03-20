#ifndef __LIBK_INCLUDE_STACK_H__
#define __LIBK_INCLUDE_STACK_H__

#include "common/include/inttypes.h"

extern void set_stack_magic(ulong stack_limit_addr);
extern void check_stack_magic(ulong stack_limit_addr);
extern void check_stack_pos(ulong stack_limit_addr);

#endif
