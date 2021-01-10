#include <stdio.h>
#include <assert.h>
#include <sys.h>

#include "system/include/exec.h"
#include "system/include/task.h"


struct startup_record {
    const char *name;
    const char *path;
    enum process_type type;
    ulong pid;
};

static struct startup_record records[] = {
    { "device", "/sys/coreimg/device.elf", PROCESS_TYPE_SYSTEM, 0 },
};

#define NUM_STARTUP_RECORDS (sizeof(records) / sizeof(struct startup_record))


static int start_task(struct startup_record *record)
{
    kprintf("Starting up %s @ %s\n", record->name, record->path);

//     // Create process
//     pid_t pid = record->pid = syscall_process_create(record->type);
//     if (!pid) {
//         return -1;
//     }
//
//     // Load ELF
//     unsigned long entry = 0, free_start = 0;
//     int err = exec_elf(pid, record->path, &entry, &free_start);
//     if (err) {
//         return -1;
//     }
//
//     // Set up env and args
//
//     // Start
//     tid_t tid = syscall_thread_create_cross(pid, entry, free_start);
//     if (!tid) {
//         return -1;
//     }

    // Create task
    char *argv[] = { (char *)record->path };
    record->pid = task_create(0, record->type, 1, argv, NULL);

    // Wait for daemon
    // TODO

    return 0;
}


void startup()
{
    kprintf("Starting up\n");

    for (int i = 0; i < NUM_STARTUP_RECORDS; i++) {
        int err = start_task(&records[i]);
        panic_if(err, "Unable to start %s @ %s\n",
                 records[i].name, records[i].path);
    }

    kprintf("All tasks started!\n");
}
