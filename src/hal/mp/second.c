#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "hal/include/kprintf.h"
#include "hal/include/hal.h"
#include "hal/include/dev.h"
#include "hal/include/mem.h"
#include "hal/include/mp.h"


/*
 * Lock
 */
static volatile ulong mp_bringup_lock = 0;
static volatile ulong start_working_lock = 1;

static inline void _mp_lock(volatile ulong *l)
{
    atomic_write(l, 1);
}

static inline void _mp_unlock(volatile ulong *l)
{
    atomic_write(l, 0);
    atomic_notify();
}

static inline void _mp_wait(volatile ulong *l)
{
    while (atomic_read(l)) {
        atomic_pause();
    }
}


/*
 * Bring up all secondary CPUs
 */
static volatile int secondry_cpus_started = 0;

static volatile ulong cur_bringup_mp_id = 0;
static volatile int cur_bringup_mp_seq = 0;

int is_any_secondary_cpu_started()
{
    return secondry_cpus_started;
}

ulong get_cur_bringup_mp_id()
{
    atomic_mb();
    return cur_bringup_mp_id;
}

int get_cur_bringup_mp_seq()
{
    atomic_mb();
    return cur_bringup_mp_seq;
}

void bringup_all_secondary_cpus()
{
    if (is_single_cpu()) {
        kprintf("Single-CPU system\n");
        return;
    }

    secondry_cpus_started = 1;
    atomic_mb();

    struct hal_arch_funcs *funcs = get_hal_arch_funcs();
    ulong entry = funcs->mp_entry;

    kprintf("Bringing up secondary processors @ %lx\n", entry);
    _mp_lock(&start_working_lock);

    // Bring up all processors
    ulong bootstrap_mp_id = arch_get_cur_mp_id();
    int num_cpus = get_num_cpus();

    for (int mp_seq = 0; mp_seq < num_cpus; mp_seq++) {
        ulong mp_id = get_mp_id_by_seq(mp_seq);
        kprintf("mp id: %lx, seq: %d, boot mp id: %lx\n",
                mp_id, mp_seq, bootstrap_mp_id);

        if (mp_id == bootstrap_mp_id) {
            panic_if(mp_seq, "Bootstrap CPU must have mp_seq == 0!\n");
            continue;
        }

        cur_bringup_mp_id = mp_id;
        cur_bringup_mp_seq = mp_seq;
        atomic_mb();

        // Bring up only 1 cpu at a time
        _mp_lock(&mp_bringup_lock);

        // Bring up the CPU
        int ok = drv_func_start_cpu(mp_seq, mp_id, entry);
        if (ok != DRV_FUNC_INVOKE_OK) {
            arch_start_cpu(mp_seq, mp_id, entry);
        }

        // Wait until the CPU is brought up
        _mp_wait(&mp_bringup_lock);
    }

    kprintf("All processors have been brought up\n");
}

/*
 * This will allow all APs start working
 */
void release_secondary_cpu_lock()
{
    _mp_unlock(&start_working_lock);
}

/*
 * Secondary processor initializaton done
 */
void secondary_cpu_init_done()
{
    kprintf("\tSecondary processor initialzied, waiting for the MP lock release\n");

    _mp_unlock(&mp_bringup_lock);
    _mp_wait(&start_working_lock);
}
