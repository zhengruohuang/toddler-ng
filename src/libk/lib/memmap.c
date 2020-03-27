#include "common/include/inttypes.h"
#include "libk/include/string.h"
#include "libk/include/debug.h"
#include "libk/include/bit.h"
#include "libk/include/kprintf.h"
#include "libk/include/memmap.h"


static int entry_count = 0;
static int entry_limit = 0;

static struct loader_memmap_entry *memmap;


/*
 * Claim a region
 */
static struct loader_memmap_entry *insert_entry(int idx)
{
    panic_if(entry_count >= entry_limit, "Memmap overflow\n");
    panic_if(idx > entry_count, "Unable to insert!\n");

    for (int i = entry_count - 1; i >= idx; i--) {
        memcpy(&memmap[i + 1], &memmap[i], sizeof(struct loader_memmap_entry));
    }

    entry_count++;

    return &memmap[idx];
}

static struct loader_memmap_entry *append_entry()
{
    return insert_entry(entry_count);
}

static void remove_entry(int idx)
{
    if (idx > entry_count) {
        return;
    }

    int i = 0;
    for (i = idx; i < entry_count - 1; i++) {
        memcpy(&memmap[i], &memmap[i + 1], sizeof(struct loader_memmap_entry));
    }
    memzero(&memmap[i], sizeof(struct loader_memmap_entry));

    entry_count--;
}

static void merge_consecutive_regions()
{
    for (int i = 0; i < entry_count - 1; ) {
        struct loader_memmap_entry *entry = &memmap[i];
        struct loader_memmap_entry *next = &memmap[i + 1];

        if (entry->start + entry->size == next->start &&
            entry->flags == next->flags
        ) {
            entry->size += next->size;
            remove_entry(i + 1);
        } else {
            i++;
        }
    }
}

void claim_memmap_region(u64 start, u64 size, int flags)
{
    //__kprintf("to claim @ %llx, size: %llx\n", start, size);

    u64 end = start + size;

    for (int i = 0; i < entry_count && size; i++) {
        struct loader_memmap_entry *entry = &memmap[i];
        u64 entry_end = entry->start + entry->size;

        if (end <= entry->start) {
            entry = insert_entry(i);

            entry->start = start;
            entry->size = size;
            entry->flags = flags;

            size = 0;
        }

        else if (start < entry->start) {
            entry = insert_entry(i);

            entry->start = start;
            entry->size = entry->start - start;
            entry->flags = flags;

            size -= entry->size;
            start += entry->size;
        }

        else if (start == entry->start) {
            if (size >= entry->size) {
                entry->flags = flags;

                size -= entry->size;
                start += entry->size;
            } else {
                entry->start += size;
                entry->size -= size;

                entry = insert_entry(i);

                entry->start = start;
                entry->size = size;
                entry->flags = flags;

                size = 0;
            }
        }

        else if (start > entry->start && start < entry_end) {
            int ori_flags = entry->flags;
            entry->size = start - entry->start;

            entry = insert_entry(i + 1);

            if (end >= entry_end) {
                entry->start = start;
                entry->size = entry_end - start;
                entry->flags = flags;

                size -= entry->size;
                start += entry->size;
            } else {
                entry->start = start + size;
                entry->size = entry_end - end;
                entry->flags = ori_flags;

                entry = insert_entry(i + 1);

                entry->start = start;
                entry->size = size;
                entry->flags = flags;

                size = 0;
                i++;
            }

            i++;
        }
    }

    if (size) {
        struct loader_memmap_entry *entry = append_entry();
        entry->start = start;
        entry->size = size;
        entry->flags = flags;
    }

    merge_consecutive_regions();
}


/*
 * Allocates and returns physical address
 *  ``align'' must be a power of 2
 */
u64 find_free_memmap_region(ulong size, ulong align)
{
    if (!size) {
        return 0;
    }

    if (align) {
        panic_if(popcount(align) > 1, "Must align to power of 2!");
    }

    for (int i = 0; i < entry_count; i++) {
        struct loader_memmap_entry *entry = &memmap[i];
        if (entry->flags == MEMMAP_USABLE && entry->size >= size) {
            u64 aligned_start = entry->start;
            if (align) {
                aligned_start = ALIGN_UP(entry->start, (u64)align);
            }

            u64 entry_end = entry->start + entry->size;
            if (aligned_start < entry_end &&
                entry_end - aligned_start >= size
            ) {
                claim_memmap_region(aligned_start, size, MEMMAP_USED);
                return aligned_start;
            }
        }
    }

    return 0;
}


/*
 * Misc
 */
void print_memmap()
{
    for (int i = 0; i < entry_count; i++) {
        struct loader_memmap_entry *entry = &memmap[i];
        __kprintf("Memory region #%d @ %llx - %llx, size: %llx, flag: %d\n", i,
            entry->start, entry->start + entry->size, entry->size, entry->flags
        );
    }
}

struct loader_memmap_entry *get_memmap(int *num_entries, int *limit)
{
    if (num_entries) {
        *num_entries = entry_count;
    }

    if (entry_limit) {
        *limit = entry_limit;
    }

    return memmap;
}

void init_libk_memmap(struct loader_memmap_entry *mmap, int num_entries, int limit)
{
    memmap = mmap;
    entry_count = num_entries;
    entry_limit = limit;
}
