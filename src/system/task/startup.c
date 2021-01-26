#include <stdio.h>
#include <assert.h>
#include <sys.h>
#include <sys/api.h>

#include "system/include/task.h"


struct startup_record {
    const char *name;
    const char *path;
    enum process_type type;
    ulong pid;
};

static struct startup_record records[] = {
    { "device", "/sys/coreimg/device.elf", PROCESS_TYPE_SYSTEM, 0 },
    { "console", "/sys/coreimg/console.elf", PROCESS_TYPE_SYSTEM, 0 },
};

#define NUM_STARTUP_RECORDS (sizeof(records) / sizeof(struct startup_record))


static int start_task(struct startup_record *record)
{
    kprintf("Starting up %s @ %s\n", record->name, record->path);

    // Create task
    char *argv[] = { (char *)record->path };
    record->pid = task_create(0, record->type, 1, argv, NULL);

    // Wait for daemon
    sys_api_task_wait(record->pid, NULL);

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
