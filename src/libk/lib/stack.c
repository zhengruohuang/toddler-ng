#include "common/include/inttypes.h"
#include "common/include/abi.h"
#include "libk/include/debug.h"

#define STACK_MAGIC 0x70dd1e2ul

void set_stack_magic(ulong stack_limit_addr)
{
    *(volatile ulong *)(void *)stack_limit_addr = 0x70dd1e2ul;
}

void check_stack_magic(ulong stack_limit_addr)
{
    ulong val = *(volatile ulong *)(void *)stack_limit_addr;
    panic_if(val != STACK_MAGIC,
             "Stack corrupted @ %lx: %lx\n", stack_limit_addr, val);
}

void check_stack_pos(ulong stack_limit_addr)
{
    ulong var = 0;

    ulong cur_stack_addr = (ulong)(void *)&var;
#if (defined(STACK_GROWS_UP) && STACK_GROWS_UP)
    long dist = stack_limit_addr, cur_stack_addr;
#else
    long dist = cur_stack_addr - stack_limit_addr;
#endif
    panic_if(dist < 256, "Stack (almost) full, dist to limit: %ld\n", dist);
}
