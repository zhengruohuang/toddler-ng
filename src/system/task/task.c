#include "common/include/inttypes.h"


struct task_file {
    int valid;
    ulong fid;
};

struct task {
    int valid;

    ulong task_id;
    ulong pid;

    ulong num_files;
    struct task_file files[32];
};


// static struct task tasks[128];


void init_task()
{
}


ulong create_task()
{
    return 0;
}
