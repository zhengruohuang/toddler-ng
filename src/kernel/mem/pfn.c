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


static ulong paddr_start = 0;
static ulong paddr_end = 0;

static ulong pfndb_entries = 0;
static ulong pfndb_offset = 0;  // First PFN
static ulong pfndb_limit = 0;   // Last PFN + 1

static struct pfndb_entry *pfndb;


/*
 * Obtain PFNDB entry
 */
struct pfndb_entry *get_pfn_entry_by_pfn(ulong pfn)
{
    panic_if(pfn < pfndb_offset || pfn >= pfndb_limit,
             "PFN (%lx) out of range: (%lx to %lx)\n", pfn, pfndb_offset, pfndb_limit);

    return &pfndb[pfn - pfndb_offset];
}

struct pfndb_entry *get_pfn_entry_by_paddr(ulong paddr)
{
    ulong pfn = ADDR_TO_PFN(paddr);
    return get_pfn_entry_by_pfn(pfn);
}


/*
 * Physical memory range
 */
// Returns mem size, start = paddr start
ulong get_mem_range(ulong *start)
{
    if (start) {
        *start = paddr_start;
    }

    return paddr_end - paddr_start;
}

// Returns pfn entry count, start = pfndb offset
ulong get_pfn_range(ulong *start)
{
    if (start) {
        *start = pfndb_offset;
    }

    return pfndb_entries;
}


/*
 * Reserve blocks of memory, only used before palloc is initialized
 */
static void reserve_pfndb_range(ulong start_pfn, ulong count)
{
    kprintf("\tReserving pages @ %lx to %lx ...",
            start_pfn, start_pfn + count);

    for (ulong i = 0; i < count; i++) {
        ulong pfn = start_pfn + i;
        struct pfndb_entry *entry = get_pfn_entry_by_pfn(pfn);

        entry->inuse = 1;
        entry->zeroed = 0;
        entry->kernel = 1;
        entry->swappable = 0;
        entry->tag = 9;

        kprintf(".");
    }

    kprintf(" done\n");
}

// Returns PFN
ulong reserve_free_pages(ulong count)
{
    for (ulong pfn = pfndb_offset; pfn < pfndb_limit - count; pfn++) {
        struct pfndb_entry *entry = get_pfn_entry_by_pfn(pfn);

        if (entry->usable && !entry->inuse) {
            int found = 1;

            for (ulong i = 1; i < count; i++) {
                entry = get_pfn_entry_by_pfn(pfn + i);
                if (!entry->usable || entry->inuse) {
                    found = 0;
                    pfn += i;
                    break;
                }
            }

            if (found) {
                reserve_pfndb_range(pfn, count);
                return pfn;
            }
        }
    }

    panic("Unable to reserve %ld free pages!\n", count);
    return 0;
}

// Returns Paddr
ulong reserve_free_mem(ulong size)
{
    return PFN_TO_ADDR(reserve_free_pages(ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE));
}


/*
 * Init
 */
static ulong find_paddr_range(ulong *start, ulong *end)
{
    struct hal_exports *hal = get_hal_exports();

    ulong small = (ulong)ALIGN_DOWN(hal->memmap[0].start, (u64)PAGE_SIZE);
    ulong big = (ulong)ALIGN_UP(hal->memmap[hal->memmap_count - 1].start +
                    hal->memmap[hal->memmap_count - 1].size, (u64)PAGE_SIZE);

    if (start) *start = small;
    if (end) *end = big;

    return big - small;
}

static ulong find_pfndb_storage(ulong pfndb_size)
{
    struct hal_exports *hal = get_hal_exports();

    // Find a contineous region to store the PFNDB
    for (int e = 0; e < hal->memmap_count; e++) {
        struct loader_memmap_entry *cur = &hal->memmap[e];

        if (cur->flags == MEMMAP_USABLE && cur->size >= pfndb_size) {
            ulong aligned_start = ALIGN_UP(cur->start ? cur->start : PAGE_SIZE, PAGE_SIZE);
            ulong aligned_end = ALIGN_DOWN(cur->start + cur->size, PAGE_SIZE);
            ulong aligned_len = aligned_end - aligned_start;

            if (aligned_len >= pfndb_size) {
                return aligned_start;
            }
        }
    }

    return 0;
}

static void construct_pfndb()
{
    struct hal_exports *hal = get_hal_exports();

    ulong prev_end = paddr_start;
    //ulong count = 0;

    for (int e = 0; e < hal->memmap_count; e++) {
        struct loader_memmap_entry *cur = &hal->memmap[e];

        ulong aligned_start = ALIGN_UP((ulong)cur->start, PAGE_SIZE);
        ulong aligned_end = ALIGN_DOWN((ulong)(cur->start + cur->size), PAGE_SIZE);

        // Fill in the hole between two zones (is there is a hole)
        if (prev_end < aligned_start) {
            kprintf("\t\tHole   @ %lx - %lx (%lx), Usable: 0\n",
                prev_end, aligned_start, aligned_start - prev_end
            );
        }

        for (ulong i = prev_end; i < aligned_start; i += PAGE_SIZE) {
            struct pfndb_entry *entry = get_pfn_entry_by_paddr(i);

            entry->usable = 0;
            entry->mapped = 0;
            entry->tag = -1;
            entry->inuse = 1;
            entry->zeroed = 0;
            entry->kernel = 1;
            entry->swappable = 0;

            // Show progress
            //if (0 == count++ % (pfndb_entries / 16)) {
            //    kprintf("_");
            //}
        }

        // Initialize the actual PFN entries
        if (aligned_start < aligned_end) {
            kprintf("\t\tRegion @ %lx - %lx (%lx), Usable: %d\n",
                aligned_start, aligned_end, aligned_end - aligned_start,
                cur->flags == MEMMAP_USABLE ? 1 : 0
            );
        }

        for (ulong i = aligned_start; i < aligned_end; i += PAGE_SIZE) {
            struct pfndb_entry *entry = get_pfn_entry_by_paddr(i);

            if (cur->flags == MEMMAP_USABLE) {
                entry->usable = 1;
                entry->mapped = 1;
                entry->tag = 0;
                entry->inuse = 0;
                entry->zeroed = 0;
                entry->kernel = 0;
                entry->swappable = 1;
            } else {
                entry->usable = 1;
                entry->mapped = 1;
                entry->tag = -1;
                entry->inuse = 1;
                entry->zeroed = 0;
                entry->kernel = 1;
                entry->swappable = 0;
            }


            // Show progress
            //if (0 == count++ % (pfndb_entries / 16)) {
            //    kprintf(".");
            //}
        }

        prev_end = aligned_end;
    }

    // Fill the rest of the PFN database
    if (prev_end < paddr_end) {
        kprintf("\t\tLast   @ %lx - %lx (%lx), Usable: 0\n",
            prev_end, paddr_end, paddr_end - prev_end
        );
    }

    for (ulong i = prev_end; i < paddr_end; i += PAGE_SIZE) {
        struct pfndb_entry *entry = get_pfn_entry_by_paddr(i);

        entry->usable = 0;
        entry->mapped = 0;
        entry->tag = -1;
        entry->inuse = 1;
        entry->zeroed = 0;
        entry->kernel = 1;
        entry->swappable = 0;
    }
}

void init_pfndb()
{
    kprintf("Initializing PFN database\n");

    // Calculate paddr range
    ulong paddr_len = find_paddr_range(&paddr_start, &paddr_end);
    kprintf("\tPaddr range: %lx - %lx\n", paddr_start, paddr_end);

    // Calculate the size of PFN DB
    pfndb_offset = paddr_start / PAGE_SIZE;
    pfndb_entries = ALIGN_UP(paddr_len, PAGE_SIZE) / PAGE_SIZE;
    pfndb_limit = ADDR_TO_PFN(paddr_end);

    ulong pfndb_size = pfndb_entries * sizeof(struct pfndb_entry);
    ulong pfndb_pages = ALIGN_UP(pfndb_size, PAGE_SIZE) / PAGE_SIZE;
    kprintf("\tPFNDB offset: %lx, PFN database size: %lx KB, pages: %ld\n",
            pfndb_offset, pfndb_size / 1024, pfndb_pages);

    // Find a contineous region to store the PFNDB
    pfndb = (void *)find_pfndb_storage(pfndb_size);
    panic_if(!pfndb, "Unable to find a region to store PFNDB!\n");

    // Construct the PFN DB
    kprintf("\tConstructing PFN database\n");
    construct_pfndb();

    // Reserve the first page, aka Paddr = 0
    if (!pfndb_offset) {
        struct pfndb_entry *entry = get_pfn_entry_by_paddr(0);
        if (entry->usable && !entry->inuse) {
            entry->usable = 0;
            entry->inuse = 1;
        }
    }

    // Mark PFN database memory as inuse
    reserve_pfndb_range(ADDR_TO_PFN((ulong)pfndb), pfndb_pages);
}
