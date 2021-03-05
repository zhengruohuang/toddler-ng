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
        "  uptime: %llu s %llu ms\n"

        "  num processes:    %lu\n"
        "  num threads:      %lu\n"
        "  num wait threads: %lu\n"
        "  num IPC threads:  %lu\n"

        "  queued TLB shootdown reqs: %lu\n"
        "  global TLB shootdown seq:  %lu\n"

        "  physical addr start @ %llx\n"
        "  physical addr end   @ %llx\n"
        "  physical addr range:  %llu\n"

        "  PFN offset: %llu\n"
        "  PFN limit:  %llu\n"
        "  PFN count:  %llu\n"

        "  num usable pages:    %llu\n"
        "  num allocated pages: %llu\n",

        (u64)((ulong)stats->kernel.uptime_ms / 1000),
        (u64)((ulong)stats->kernel.uptime_ms % 1000),

        stats->kernel.num_procs,
        stats->kernel.num_threads,
        stats->kernel.num_threads_wait,
        stats->kernel.num_threads_ipc,

        stats->kernel.num_tlb_shootdown_reqs,
        stats->kernel.global_tlb_shootdown_seq,

        stats->kernel.paddr_start,
        stats->kernel.paddr_end,
        stats->kernel.paddr_len,

        stats->kernel.pfn_offset,
        stats->kernel.pfn_limit,
        stats->kernel.pfn_count,

        stats->kernel.num_pages_usable,
        stats->kernel.num_pages_allocated
    );

    kprintf("  num salloc objs: %lu\n", stats->kernel.num_salloc_objs);
    for (ulong i = 0; i < stats->kernel.num_salloc_objs; i++) {
        kprintf("    #%d: %s, block size: %lu, num objs: %lu, num pages: %llu\n", i,
                stats->kernel.salloc_objs[i].name,
                stats->kernel.salloc_objs[i].block_size,
                stats->kernel.salloc_objs[i].num_objs,
                stats->kernel.salloc_objs[i].num_pages_allocated);
    }

    kprintf("  num processes: %lu\n", stats->kernel.num_procs);
    for (ulong i = 0; i < stats->kernel.num_procs; i++) {
        kprintf("    #%d: %s, VM blocks: %lu, num pages: %lu\n", i,
                stats->kernel.procs[i].name,
                stats->kernel.procs[i].num_inuse_vm_blocks,
                stats->kernel.procs[i].num_pages);
    }
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
