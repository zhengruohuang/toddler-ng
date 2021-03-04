/*
 * Stats syscall handlers
 */


#include "common/include/inttypes.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/proc.h"
#include "kernel/include/lib.h"
#include "kernel/include/mem.h"
#include "kernel/include/kernel.h"
#include "kernel/include/syscall.h"


/*
 * Kernel
 */
int syscall_handler_stats_kernel(struct process *p, struct thread *t,
                                 struct kernel_dispatch *kdi)
{
    ulong vaddr = kdi->param0;
    paddr_t paddr = get_hal_exports()->translate(p->page_table, vaddr);
    struct kernel_stats *stats = hal_cast_paddr_to_kernel_ptr(paddr);

    // Time
    stats->uptime_ms = hal_get_ms();

    // Proc
    stats->num_procs = get_num_processes();
    stats->num_threads = get_num_threads();
    stats->num_threads_wait = get_num_wait_threads();
    stats->num_threads_ipc = get_num_ipc_threads();
    process_stats(&stats->num_procs, stats->procs, NUM_PROC_STATS);

    // TLB
    tlb_shootdown_stats(&stats->num_tlb_shootdown_reqs, &stats->global_tlb_shootdown_seq);

    // PFN
    paddr_t paddr_start = 0, paddr_end = 0;
    paddr_t paddr_len = get_mem_range(&paddr_start, &paddr_end);
    stats->paddr_start = paddr_start;
    stats->paddr_end = paddr_end;
    stats->paddr_len = paddr_len;

    ppfn_t pfndb_offset, pfndb_limit = 0;
    ppfn_t pfndb_entries = get_pfn_range(&pfndb_offset, &pfndb_limit);
    stats->pfn_offset = pfndb_offset;
    stats->pfn_limit = pfndb_limit;
    stats->pfn_count = pfndb_entries;

    // Palloc
    palloc_stats_page(&stats->num_pages_usable, &stats->num_pages_allocated);

    // Salloc
    salloc_stats(&stats->num_salloc_objs, stats->salloc_objs, NUM_SALLOC_OBJ_STATS);

    hal_set_syscall_return(kdi->regs, 0, 0, 0);
    return SYSCALL_HANDLED_CONTINUE;
}
