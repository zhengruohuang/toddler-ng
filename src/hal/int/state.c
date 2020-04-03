#include "hal/include/mp.h"
#include "hal/include/lib.h"
#include "hal/include/hal.h"


/*
 * Get/Set state
 */
static decl_per_cpu(int, interrupt_enabled);

int get_local_int_state()
{
    volatile int *ptr = get_per_cpu(int, interrupt_enabled);
    int enabled = *ptr;

    return enabled;
}

void set_local_int_state(int enabled)
{
    volatile int *ptr = get_per_cpu(int, interrupt_enabled);
    *ptr = enabled;
}


/*
 * Disable/Enable/Restore
 */
int disable_local_int()
{
    arch_disable_local_int();

    int enabled = get_local_int_state();
    set_local_int_state(0);

    return enabled;
}

void enable_local_int()
{
    set_local_int_state(1);

    arch_enable_local_int();
}

int restore_local_int(int enabled)
{
    int cur_state = get_local_int_state();

    if (cur_state) {
        assert(enabled);
    }

    else if (enabled) {
        enable_local_int();
    }

    return cur_state;
}


/*
 * Init
 */
void init_int_state()
{
    set_local_int_state(0);
}

void init_int_state_mp()
{
    set_local_int_state(0);
}
