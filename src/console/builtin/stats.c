#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/api.h>


/*
 * Buf
 */
struct stats_buf {
    struct kernel_stats kernel;
};

static struct stats_buf *stats = NULL;

static void init_stats()
{
    stats = (void *)syscall_vm_map(VM_MAP_KERNEL, 0, sizeof(struct stats_buf));
}


/*
 * Stats
 */
static void stats_kernel()
{
    if (!stats) {
        return;
    }

    syscall_stats_kernel(&stats->kernel);

    kprintf(
        "Kernel statistics:\n"

        "  num processes:    %lu\n"
        "  num threads:      %lu\n"
        "  num wait threads: %lu\n"
        "  num IPC threads:  %lu\n"

        "  physical addr start @ %llx\n"
        "  physical addr end   @ %llx\n"
        "  physical addr range:  %llu\n"

        "  PFN offset: %llu\n"
        "  PFN limit:  %llu\n"
        "  PFN count:  %llu\n"

        "  num usable pages:    %llu\n"
        "  num allocated pages: %llu\n",

        stats->kernel.num_procs,
        stats->kernel.num_threads,
        stats->kernel.num_threads_wait,
        stats->kernel.num_threads_ipc,

        stats->kernel.paddr_start,
        stats->kernel.paddr_end,
        stats->kernel.paddr_len,

        stats->kernel.pfn_offset,
        stats->kernel.pfn_limit,
        stats->kernel.pfn_count,

        stats->kernel.num_pages_usable,
        stats->kernel.num_pages_allocated
    );
}


/*
 * Command
 */
int exec_stats(int argc, char **argv)
{
    if (!stats) {
        init_stats();
    }

    if (argc > 1 && argv[1]) {
        if (!strcmp(argv[1], "kernel")) {
            stats_kernel();
        }
    } else {
        stats_kernel();
    }

    return 0;
}
