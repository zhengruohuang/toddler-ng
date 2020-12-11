/*
 * Start up user space
 */


#include "common/include/inttypes.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/lib.h"
#include "kernel/include/syscall.h"
#include "kernel/include/proc.h"


struct startup_record {
    char *name;
    char *filename;
    char *url;
    enum process_type type;
    ulong pid;
};


static struct startup_record records[] = {
    { "system", "tdlrsys.elf", "coreimg://tdlrsys.elf", PROCESS_TYPE_SYSTEM, 0 },
};


static void start_load_and_run_process(struct startup_record *record)
{
    record->pid = create_process(0, record->name, record->type);

    access_process(record->pid, p) {
        void *elf = coreimg_find_file(record->filename);
        load_coreimg_elf(p, record->url, elf);

        create_and_run_thread(tid, t, p, p->vm.entry_point, 0, NULL) {
            kprintf("\tSystem process ID: %x, thread ID: %x\n", record->pid, tid);
        }
    }
}

static void startup_worker(ulong param)
{
    for (int i = 0; i < sizeof(records) / sizeof(struct startup_record); i++) {
        struct startup_record *record = &records[i];
        start_load_and_run_process(record);
    }

    // Terminate self
    syscall_thread_exit_self(0);
}


void init_startup()
{
    create_and_run_kernel_thread(tid, t, &startup_worker, 0, NULL) {
        kprintf("\tStartup worker created, thread ID: %p, thread block base: %p\n", tid, t->memory.block_base);
    }
}
