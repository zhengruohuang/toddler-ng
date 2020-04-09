#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/mp.h"
#include "hal/include/mem.h"
#include "hal/include/hal.h"


#define PER_CPU_AREA_PAGE_COUNT     1
#define PER_CPU_AREA_SIZE           (PAGE_SIZE * PER_CPU_AREA_PAGE_COUNT)
#define PER_CPU_DATA_START_OFFSET   ((PER_CPU_AREA_SIZE / 2) + 16)
#define PER_CPU_STACK_TOP_OFFSET    ((PER_CPU_AREA_SIZE / 2) - 16)


static paddr_t per_cpu_area_start_paddr = 0;
static ulong per_cpu_area_start_vaddr = 0;

static int cur_per_cpu_offset = 0;


/*
 * Access per-CPU vars
 */
static ulong get_per_cpu_area_start_vaddr(int cpu_seq)
{
    assert(cpu_seq < get_num_cpus());
    return per_cpu_area_start_vaddr + PER_CPU_AREA_SIZE * cpu_seq;
}

ulong get_my_cpu_area_start_vaddr()
{
    return get_per_cpu_area_start_vaddr(get_cur_mp_seq());
}

ulong get_my_cpu_data_area_start_vaddr()
{
    return get_my_cpu_area_start_vaddr() + PER_CPU_DATA_START_OFFSET;
}

ulong get_my_cpu_stack_top_vaddr()
{
    return get_my_cpu_area_start_vaddr() + PER_CPU_STACK_TOP_OFFSET;
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

    return (void *)(get_my_cpu_data_area_start_vaddr() + *offset);
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

    ppfn_t per_cpu_are_start_ppfn = pre_palloc(per_cpu_page_count);
    per_cpu_area_start_paddr = ppfn_to_paddr(per_cpu_are_start_ppfn);
    per_cpu_area_start_vaddr = pre_valloc(per_cpu_page_count, per_cpu_area_start_paddr, 1);

    // Map per CPU private area
    paddr_t cur_area_pstart = per_cpu_area_start_paddr;
    ulong cur_area_vstart = per_cpu_area_start_vaddr;

    kprintf("\tPer-CPU area start phys @ %llx, virt @ %lx\n",
            (u64)cur_area_pstart, cur_area_vstart);

    for (int i = 0; i < num_cpus; i++) {
        // Map the page to its new virt location
        //hal_map_range(cur_area_vstart, cur_area_pstart, PER_CPU_AREA_SIZE, 1);

        // Zero the area
        memzero((void *)cur_area_vstart, PER_CPU_AREA_SIZE);

        // Tell the user
        kprintf("\tCPU #%d, per-CPU area: %lx -> %llx (%d bytes)\n", i,
            cur_area_vstart, (u64)cur_area_pstart, PER_CPU_AREA_SIZE
        );

        cur_area_pstart += PER_CPU_AREA_SIZE;
        cur_area_vstart += PER_CPU_AREA_SIZE;
    }

    kprintf("Per-cpu area init done!\n");
}
