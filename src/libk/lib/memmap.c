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

static struct loader_memmap_entry *split_entry(int idx, u64 offset)
{
    struct loader_memmap_entry *entry = &memmap[idx];
    panic_if(offset >= entry->size, "Unable to split the entry!\n");

    struct loader_memmap_entry *next_entry = insert_entry(idx + 1);
    memcpy(next_entry, entry, sizeof(struct loader_memmap_entry));

    entry->size = offset;

    next_entry->size -= offset;
    next_entry->start += offset;

    return next_entry;
}

static struct loader_memmap_entry *split_entry2(int idx, u64 offset1, u64 offset2)
{
    struct loader_memmap_entry *entry = &memmap[idx];
    panic_if(offset1 >= entry->size ||
             offset2 >= entry->size ||
             offset1 >= offset2, "Unable to split the entry!\n");

    struct loader_memmap_entry *entry3 = insert_entry(idx + 1);
    memcpy(entry3, entry, sizeof(struct loader_memmap_entry));
    entry3->size -= offset2;
    entry3->start += offset2;

    struct loader_memmap_entry *entry2 = insert_entry(idx + 1);
    memcpy(entry2, entry, sizeof(struct loader_memmap_entry));
    entry2->size = offset2 - offset1;
    entry2->start += offset1;

    entry->size = offset1;

    return entry2;
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

void claim_memmap_region(u64 start, u64 size, int type)
{
    //__kprintf("to claim @ %llx, size: %llx\n", start, size);

    u64 end = start + size;

    for (int i = 0; i < entry_count && size; i++) {
        struct loader_memmap_entry *entry = &memmap[i];
        u64 entry_end = entry->start + entry->size;

        // Insert an entry before the current one
        if (end <= entry->start) {
            entry = insert_entry(i);

            entry->start = start;
            entry->size = size;
            entry->flags = type;

            size = 0;
        }

        // The claimed region starts before the current entry,
        // but spans partial or the entire entry,
        // need to insert a partial region before current entry
        else if (start < entry->start) {
            entry = insert_entry(i);

            entry->start = start;
            entry->size = entry->start - start;
            entry->flags = type;

            size -= entry->size;
            start += entry->size;
        }

        // The region starts right at the beginning of current entry,
        // but may span partial or entire entry
        else if (start == entry->start) {
            // The region spans the entire entry
            if (size >= entry->size) {
                entry->flags = type;

                size -= entry->size;
                start += entry->size;
            }

            // The region spans only partial entry, need to split the entry
            // Also, override the type for the first half
            else {
                split_entry(i, size);
                entry->flags = type;
                size = 0;
            }
        }

        // The region starts in the middle of the entry,
        // may span partial or entire rest of the entry
        else if (start > entry->start && start < entry_end) {
            // The region spans the entire rest of the entry
            // Need to split the entry and take the second half
            if (end >= entry_end) {
                entry = split_entry(i, start - entry->start);
                entry->flags = type;

                size -= entry->size;
                i++;
            }

            // The region spans only partial of the rest of the entry
            // Need to split the region twice and take the middle part
            else {
                entry = split_entry2(i, start - entry->start, end - entry->start);
                entry->flags = type;

                size = 0;
                i += 2;
            }
        }
    }

    // The region starts after the last entry, simply append a new entry
    if (size) {
        struct loader_memmap_entry *entry = append_entry();
        entry->start = start;
        entry->size = size;
        entry->flags = type;
    }

    merge_consecutive_regions();
}


/*
 * Tag/Untag a region
 */
static void do_tag_memmap_region(u64 start, u64 size, u32 mask, int tag)
{
    u64 end = start + size;

    for (int i = 0; i < entry_count && size; i++) {
        struct loader_memmap_entry *entry = &memmap[i];
        u64 entry_end = entry->start + entry->size;

        // Split the entry and tag the second half
        if (entry->start < start && entry_end < end) {
            entry = split_entry(i, start - entry->start);
            if (tag) entry->tags |=  mask;
            else     entry->tags &= ~mask;
            i++;
        }

        // Split the entry and tag the first half
        else if (entry->start > start && entry_end > end) {
            split_entry(i, start - entry->start);
            if (tag) entry->tags |=  mask;
            else     entry->tags &= ~mask;
        }

        // Split the entry twice
        else if (entry->start < start && entry_end > end) {
            entry = split_entry2(i, start - entry->start, end - entry->start);
            if (tag) entry->tags |=  mask;
            else     entry->tags &= ~mask;
            i++;
        }

        // No split
        else if (entry->start >= start && entry_end <= end) {
            if (tag) entry->tags |=  mask;
            else     entry->tags &= ~mask;
        }
    }
}

void tag_memmap_region(u64 start, u64 size, u32 mask)
{
    do_tag_memmap_region(start, size, mask, 1);
}

void untag_memmap_region(u64 start, u64 size, u32 mask)
{
    do_tag_memmap_region(start, size, mask, 0);
}


/*
 * Allocates and returns physical address
 *  ``align'' must be a power of 2
 */
u64 find_free_memmap_region(u64 size, u64 align, u32 mask, int match)
{
    panic_if(popcount64(align) > 1, "Must align to power of 2!");

    for (int i = 0; i < entry_count; i++) {
        struct loader_memmap_entry *entry = &memmap[i];
        if (entry->flags == MEMMAP_USABLE && entry->size >= size) {
            u64 aligned_start = entry->start;
            if (align) {
                aligned_start = ALIGN_UP(entry->start, (u64)align);
            }

            u64 entry_end = entry->start + entry->size;
            if (aligned_start < entry_end &&
                entry_end - aligned_start >= size   // Avoid data type overflow
            ) {
                int matched = 0;
                switch (match) {
                case MEMMAP_ALLOC_MATCH_IGNORE:
                    matched = 1;
                    break;
                case MEMMAP_ALLOC_MATCH_EXACT:
                    matched = entry->tags == mask ? 1 : 0;
                    break;
                case MEMMAP_ALLOC_MATCH_SET_ALL:
                    matched = (entry->tags & mask) == mask ? 1 : 0;
                    break;
                case MEMMAP_ALLOC_MATCH_SET_ANY:
                    matched = entry->tags & mask ? 1 : 0;
                    break;
                case MEMMAP_ALLOC_MATCH_UNSET_ALL:
                    matched = (~entry->tags & mask) == mask ? 1 : 0;
                    break;
                case MEMMAP_ALLOC_MATCH_UNSET_ANY:
                    matched = ~entry->tags & mask ? 1 : 0;
                    break;
                default:
                    break;
                }

                if (matched) {
                    claim_memmap_region(aligned_start, size, MEMMAP_USED);
                    return aligned_start;
                }
            }
        }
    }

    return 0;
}

u64 find_free_memmap_region_for_palloc(u64 size, u64 align)
{
    static int tags[] = {
        MEMMAP_ALLOC_MATCH_EXACT,   MEMMAP_TAG_NORMAL | MEMMAP_TAG_DIRECT_MAPPED | MEMMAP_TAG_DIRECT_ACCESS,
        MEMMAP_ALLOC_MATCH_SET_ALL, MEMMAP_TAG_NORMAL | MEMMAP_TAG_DIRECT_MAPPED | MEMMAP_TAG_DIRECT_ACCESS,
        MEMMAP_ALLOC_MATCH_EXACT,   MEMMAP_TAG_NORMAL | MEMMAP_TAG_DIRECT_MAPPED,
        MEMMAP_ALLOC_MATCH_SET_ALL, MEMMAP_TAG_NORMAL | MEMMAP_TAG_DIRECT_MAPPED,
        MEMMAP_ALLOC_MATCH_EXACT,   MEMMAP_TAG_NORMAL,
        MEMMAP_ALLOC_MATCH_SET_ALL, MEMMAP_TAG_NORMAL,
        MEMMAP_ALLOC_MATCH_IGNORE,  0,
    };

    u64 paddr = 0;
    for (int t = 0; t < sizeof(tags) / sizeof(int) && !paddr; t += 2) {
        warn_if(tags[t] == MEMMAP_ALLOC_MATCH_IGNORE,
                "Unable to find memory region with the proper tags set!\n");
        paddr = find_free_memmap_region(size, align, tags[t + 1], tags[t]);
    }

    panic_if(!paddr, "Unable to allocate phys mem!\n");
    return paddr;
}


/*
 * Misc
 */
void print_memmap()
{
    for (int i = 0; i < entry_count; i++) {
        struct loader_memmap_entry *entry = &memmap[i];
        __kprintf("Memory region #%d @ %llx - %llx, "
                  "size: %llx, flag: %d, tags: %b\n", i,
                  entry->start, entry->start + entry->size,
                  entry->size, entry->flags, entry->tags
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
