#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/mp.h"
#include "hal/include/mem.h"
#include "hal/include/hal.h"


static ulong per_cpu_area_start_paddr = 0;
static ulong per_cpu_area_start_vaddr = 0;

static int cur_per_cpu_offset = 0;


/*
 * Access per-CPU vars
 */
static ulong get_per_cpu_area_start_vaddr(int cpu_id)
{
    assert(cpu_id < get_num_cpus());
    return per_cpu_area_start_vaddr + PER_CPU_AREA_SIZE * cpu_id;
}

ulong get_my_cpu_area_start_vaddr()
{
    return get_per_cpu_area_start_vaddr(arch_get_cpu_index());
}

static void init_per_cpu_var(int *offset, size_t size)
{
    assert(cur_per_cpu_offset + size < PAGE_SIZE);

    *offset = cur_per_cpu_offset;
    cur_per_cpu_offset += ALIGN_UP(size, sizeof(ulong));
}

void *access_per_cpu_var(int *offset, size_t size)
{
    if (*offset == -1) {
        init_per_cpu_var(offset, size);
    }

    return (void *)(get_my_cpu_area_start_vaddr() + PER_CPU_DATA_START_OFFSET + *offset);
}


/*
 * Init
 */
void init_per_cpu_area()
{
    kprintf("Initializing per-cpu area\n");

    // Reserve pages
    int num_cpus = get_num_cpus();
    int per_cpu_page_count = PER_CPU_AREA_PAGE_COUNT * num_cpus;
    ulong per_cpu_are_start_pfn = pre_palloc(per_cpu_page_count);
    per_cpu_area_start_paddr = PFN_TO_ADDR(per_cpu_are_start_pfn);
    per_cpu_area_start_vaddr = pre_valloc(per_cpu_page_count, per_cpu_area_start_paddr, 1);

    // Map per CPU private area
    ulong cur_area_pstart = per_cpu_area_start_paddr;
    ulong cur_area_vstart = per_cpu_area_start_vaddr;

    kprintf("\tPer-CPU area start phys @ %lx, virt @ %lx\n",
            cur_area_pstart, cur_area_vstart);

    for (int i = 0; i < num_cpus; i++) {
        // Map the page to its new virt location
        //hal_map_range(cur_area_vstart, cur_area_pstart, PER_CPU_AREA_SIZE, 1);

        // Zero the area
        memzero((void *)cur_area_vstart, PER_CPU_AREA_SIZE);

        // Tell the user
        kprintf("\tCPU #%d, per-CPU area: %lx -> %lx (%d bytes)\n", i,
            cur_area_vstart, cur_area_pstart, PER_CPU_AREA_SIZE
        );

        cur_area_pstart += PER_CPU_AREA_SIZE;
        cur_area_vstart += PER_CPU_AREA_SIZE;
    }

    kprintf("Per-cpu area init done!\n");
}
