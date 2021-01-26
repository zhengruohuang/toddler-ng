/*
 * Page frame number database
 */


#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "kernel/include/kernel.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/lib.h"
#include "kernel/include/mem.h"
#include "libk/include/memmap.h"


static psize_t paddr_len = 0;
static paddr_t paddr_start = 0;
static paddr_t paddr_end = 0;

static ppfn_t pfndb_entries = 0;
static ppfn_t pfndb_offset = 0;  // First PFN
static ppfn_t pfndb_limit = 0;   // Last PFN + 1

static struct pfndb_entry *pfndb;


/*
 * Obtain PFNDB entry
 */
struct pfndb_entry *get_pfn_entry_by_pfn(ppfn_t pfn)
{
    panic_if(pfn < pfndb_offset || pfn >= pfndb_limit,
             "PFN (%llx) out of range: (%lx to %lx)\n",
             (u64)pfn, pfndb_offset, pfndb_limit);

    return &pfndb[pfn - pfndb_offset];
}

struct pfndb_entry *get_pfn_entry_by_paddr(paddr_t paddr)
{
    ppfn_t pfn = paddr_to_ppfn(paddr);
    return get_pfn_entry_by_pfn(pfn);
}


/*
 * Physical memory range
 */
// Returns mem size, start = paddr start
psize_t get_mem_range(paddr_t *start, paddr_t *end)
{
    if (start) *start = paddr_start;
    if (end) *end = paddr_end;

    return paddr_len;
}

// Returns pfn entry count, start = pfndb offset
ppfn_t get_pfn_range(ppfn_t *start, ppfn_t *end)
{
    if (start) *start = pfndb_offset;
    if (end) *end = pfndb_limit;

    return pfndb_entries;
}


/*
 * Init
 */
static psize_t find_paddr_range(paddr_t *start, paddr_t *end)
{
    struct loader_memmap *memmap = get_memmap();

    u64 start64 = memmap->entries[0].start;
    u64 end64 = memmap->entries[memmap->num_entries - 1].start +
                    memmap->entries[memmap->num_entries - 1].size;

    paddr_t lower = align_down_paddr(cast_u64_to_paddr(start64), PAGE_SIZE);
    paddr_t upper = align_up_paddr(cast_u64_to_paddr(end64), PAGE_SIZE);

    if (start) *start = lower;
    if (end) *end = upper;

    return upper - lower;
}

static void construct_pfndb()
{
    struct loader_memmap *memmap = get_memmap();
    paddr_t prev_end = paddr_start;

    for (int e = 0; e < memmap->num_entries; e++) {
        struct loader_memmap_entry *cur = &memmap->entries[e];

        paddr_t aligned_start = align_up_paddr(cast_u64_to_paddr(cur->start), PAGE_SIZE);
        paddr_t aligned_end = align_down_paddr(cast_u64_to_paddr(cur->start + cur->size), PAGE_SIZE);

        // Fill in the hole between two zones (is there is a hole)
        if (prev_end < aligned_start) {
            kprintf("\t\tHole   @ %llx - %llx (%llx), Usable: 0\n",
                    (u64)prev_end, (u64)aligned_start,
                    (u64)(aligned_start - prev_end));
        }

        for (paddr_t i = prev_end; i < aligned_start; i += PAGE_SIZE) {
            struct pfndb_entry *entry = get_pfn_entry_by_paddr(i);

            entry->usable = 0;
            entry->mapped = 0;
            entry->tags = 0;
            entry->inuse = 1;
            entry->zeroed = 0;
            entry->kernel = 1;
            entry->swappable = 0;
        }

        // Initialize the actual PFN entries
        if (aligned_start < aligned_end) {
            kprintf("\t\tRegion @ %llx - %llx (%llx), Usable: %d\n",
                    (u64)aligned_start, (u64)aligned_end,
                    (u64)(aligned_end - aligned_start),
                    (u64)cur->flags == MEMMAP_USABLE ? 1 : 0);
        }

        for (paddr_t i = aligned_start; i < aligned_end; i += PAGE_SIZE) {
            struct pfndb_entry *entry = get_pfn_entry_by_paddr(i);

            if (cur->flags == MEMMAP_USABLE) {
                entry->usable = 1;
                entry->mapped = 1;
                entry->tags = cur->tags;
                entry->inuse = 0;
                entry->zeroed = 0;
                entry->kernel = 0;
                entry->swappable = 1;
            } else {
                entry->usable = 1;
                entry->mapped = 1;
                entry->tags = 0;
                entry->inuse = 1;
                entry->zeroed = 0;
                entry->kernel = 1;
                entry->swappable = 0;
            }
        }

        prev_end = aligned_end;
    }

    // Fill the rest of the PFN database
    if (prev_end < paddr_end) {
        kprintf("\t\tLast   @ %llx - %llx (%llx), Usable: 0\n",
                (u64)prev_end, (u64)paddr_end,
                (u64)(paddr_end - prev_end));
    }

    for (paddr_t i = prev_end; i < paddr_end; i += PAGE_SIZE) {
        struct pfndb_entry *entry = get_pfn_entry_by_paddr(i);

        entry->usable = 0;
        entry->mapped = 0;
        entry->tags = 0;
        entry->inuse = 1;
        entry->zeroed = 0;
        entry->kernel = 1;
        entry->swappable = 0;
    }
}

void reserve_pfndb()
{
    // Calculate paddr range
    paddr_len = find_paddr_range(&paddr_start, &paddr_end);
    kprintf("\tPaddr range: %llx - %llx\n", (u64)paddr_start, (u64)paddr_end);

    // Calculate the PFNDB size
    pfndb_offset = paddr_to_ppfn(paddr_start);
    pfndb_limit = paddr_to_ppfn(paddr_end);
    pfndb_entries = pfndb_limit - pfndb_offset;

    ulong pfndb_bytes = pfndb_entries * sizeof(struct pfndb_entry);
    ulong aligned_pfndb_bytes = align_up_vsize(pfndb_bytes, PAGE_SIZE);

    kprintf("\tPFNDB offset: %llx, PFN database size: %lx KB, pages: %ld\n",
            (u64)pfndb_offset, pfndb_bytes / 1024, aligned_pfndb_bytes / PAGE_SIZE);

    // Find a contineous region to store the PFNDB
    u64 pfndb_u64 = find_free_memmap_direct_mapped_region(aligned_pfndb_bytes, PAGE_SIZE);
    paddr_t pfndb_paddr = cast_u64_to_paddr(pfndb_u64);
    pfndb = cast_paddr_to_ptr(pfndb_paddr);

    panic_if(!pfndb, "Unable to find a region to store PFNDB!\n");

    kprintf("\tMemory reserved for PFNDB @ %llx, size: %llx bytes\n",
        (u64)pfndb_u64, (u64)aligned_pfndb_bytes);
}

void init_pfndb()
{
    kprintf("Initializing PFN database\n");

    // Construct the PFN DB
    construct_pfndb();

    // Reserve the first page, aka Paddr = 0
    if (!pfndb_offset) {
        struct pfndb_entry *entry = get_pfn_entry_by_paddr(0);
        if (entry->usable && !entry->inuse) {
            entry->usable = 0;
            entry->inuse = 1;
        }
    }
}
