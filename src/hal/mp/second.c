#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "hal/include/kprintf.h"
#include "hal/include/hal.h"
#include "hal/include/dev.h"
#include "hal/include/mem.h"
#include "hal/include/mp.h"


static volatile ulong ap_bringup_lock = 0;
static volatile ulong start_working_lock = 1;


/*
 * Single-CPU
 */
static volatile int single_cpu = 1;

int is_single_cpu()
{
    return single_cpu;
}


/*
 * Bring up all secondary CPUs
 */
void bringup_all_secondary_cpus()
{
    int num_cpus = get_num_cpus();
    if (num_cpus > 1) {
        single_cpu = 0;
    }

    struct hal_arch_funcs *funcs = get_hal_arch_funcs();
    ulong entry = funcs->mp_entry;

    kprintf("Bringing up secondary processors @ %lx\n", entry);

    atomic_write(&start_working_lock, 1);

    // Bring up all processors
    for (int i = 0; i < num_cpus; i++) {
        if (i != get_cur_mp_seq()) {
            ulong mp_id = get_mp_id_by_seq(i);

            // Bring up only 1 cpu at a time
            atomic_write(&ap_bringup_lock, 1);

            // Bring up the CPU
            int ok = drv_func_start_cpu(i, mp_id, entry);
            if (ok != DRV_FUNC_INVOKE_OK) {
                arch_start_cpu(i, mp_id, entry);
            }

            // What until the CPU is brought up
            while (atomic_read(&ap_bringup_lock));
        }
    }

    kprintf("All processors have been brought up\n");
}

/*
 * This will allow all APs start working
 */
void release_secondary_cpu_lock()
{
    atomic_write(&start_working_lock, 0);
}

/*
 * Secondary processor initializaton done
 */
void secondary_cpu_init_done()
{
    kprintf("\tSecondary processor initialzied, waiting for the MP lock release\n");

    atomic_write(&ap_bringup_lock, 0);
    while (atomic_read(&start_working_lock));

    // TODO: Enable local interrupt
}
