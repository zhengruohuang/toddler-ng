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
             "PFN (%lx) out of range: (%lx to %lx)\n", pfn, pfndb_offset, pfndb_limit);

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
psize_t get_mem_range(paddr_t *start)
{
    if (start) {
        *start = paddr_start;
    }

    return paddr_end - paddr_start;
}

// Returns pfn entry count, start = pfndb offset
ppfn_t get_pfn_range(ppfn_t *start)
{
    if (start) {
        *start = pfndb_offset;
    }

    return pfndb_entries;
}


/*
 * Reserve blocks of memory, only used before palloc is initialized
 */
static void reserve_pfndb_range(ppfn_t start_pfn, ppfn_t count)
{
    kprintf("\tReserving pages @ %lx to %lx ...",
            start_pfn, start_pfn + count);

    for (ppfn_t i = 0; i < count; i++) {
        ppfn_t pfn = start_pfn + i;
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
ppfn_t reserve_free_pages(ppfn_t count)
{
    for (ppfn_t pfn = pfndb_offset; pfn < pfndb_limit - count; pfn++) {
        struct pfndb_entry *entry = get_pfn_entry_by_pfn(pfn);

        if (entry->usable && !entry->inuse) {
            int found = 1;

            for (ppfn_t i = 1; i < count; i++) {
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
paddr_t reserve_free_mem(psize_t size)
{
    ppfn_t page_count = get_ppage_count(size);
    ppfn_t ppfn = reserve_free_pages(page_count);
    return ppfn_to_paddr(ppfn);
}


/*
 * Init
 */
static psize_t find_paddr_range(paddr_t *start, paddr_t *end)
{
    struct hal_exports *hal = get_hal_exports();

    u64 start64 = hal->memmap[0].start;
    u64 end64 = hal->memmap[hal->memmap_count - 1].start +
                    hal->memmap[hal->memmap_count - 1].size;

    paddr_t lower = align_down_paddr(cast_u64_to_paddr(start64), PAGE_SIZE);
    paddr_t upper = align_up_paddr(cast_u64_to_paddr(end64), PAGE_SIZE);

    if (start) *start = lower;
    if (end) *end = upper;

    return upper - lower;
}

static paddr_t find_pfndb_storage(psize_t pfndb_size)
{
    struct hal_exports *hal = get_hal_exports();

    // Find a contineous region to store the PFNDB
    for (int e = 0; e < hal->memmap_count; e++) {
        struct loader_memmap_entry *cur = &hal->memmap[e];

        if (cur->flags == MEMMAP_USABLE && cur->size >= pfndb_size) {
            paddr_t start = cast_u64_to_paddr(cur->start ? cur->start : PAGE_SIZE);
            paddr_t end = cast_u64_to_paddr(cur->start + cur->size);

            paddr_t aligned_start = align_up_paddr(start, PAGE_SIZE);
            paddr_t aligned_end = align_down_paddr(end, PAGE_SIZE);

            if (aligned_end > aligned_start &&
                aligned_end - aligned_start >= pfndb_size
            ) {
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
    pfndb_limit = paddr_to_ppfn(paddr_end);

    ulong pfndb_size = pfndb_entries * sizeof(struct pfndb_entry);
    ulong pfndb_pages = ALIGN_UP(pfndb_size, PAGE_SIZE) / PAGE_SIZE;
    kprintf("\tPFNDB offset: %lx, PFN database size: %lx KB, pages: %ld\n",
            pfndb_offset, pfndb_size / 1024, pfndb_pages);

    // Find a contineous region to store the PFNDB
    paddr_t pfndb_paddr = find_pfndb_storage(pfndb_size);
    pfndb = cast_paddr_to_ptr(pfndb_paddr);
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
    reserve_pfndb_range(paddr_to_ppfn(pfndb_paddr), pfndb_pages);
}
