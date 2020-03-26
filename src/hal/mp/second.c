#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "hal/include/kprintf.h"
#include "hal/include/hal.h"
#include "hal/include/mp.h"


static volatile ulong ap_bringup_lock = 0;
static volatile ulong start_working_lock = 1;


/*
 * Bring up all secondary CPUs
 */
void bringup_all_secondary_cpus()
{
    kprintf("Bringing up secondary processors\n");

    atomic_write(&start_working_lock, 1);

    // Bring up all processors
    int i, num_cpus = get_num_cpus();
    for (i = 0; i < num_cpus; i++) {
        if (i != arch_get_cpu_index()) {
            // Bring up only 1 cpu at a time
            atomic_write(&ap_bringup_lock, 1);

            // Bring up the CPU
            arch_bringup_cpu(i);

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
