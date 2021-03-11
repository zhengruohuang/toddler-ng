/*
 * Page Frame Allocator
 */


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/kernel.h"
#include "kernel/include/atomic.h"
#include "kernel/include/lib.h"
#include "kernel/include/mem.h"


// Up to 2^15 = 32768 pages, 128MB assuming 4KB pages
#define PALLOC_ORDER_BITS       4
#define PALLOC_ORDER_COUNT      (1 << PALLOC_ORDER_BITS)
#define PALLOC_MIN_ORDER        0
#define PALLOC_MAX_ORDER        (PALLOC_ORDER_COUNT - 1)

// Up to 32 regions
#define PALLOC_REGION_BITS      5
#define MAX_PALLOC_REGION_COUNT (1 << PALLOC_REGION_BITS)


// Every physical page has a node
struct palloc_node {
    union {
        ulong value;

        struct {
            ulong order     : PALLOC_ORDER_BITS;
            ulong region    : PALLOC_REGION_BITS;
            ulong alloc     : 1;
            ulong has_next  : 1;
            ulong avail     : 1;
            ulong next      : sizeof(ulong) * 8 - PAGE_BITS;
        };
    };
};

// An allocation region, with particular tags
struct palloc_region {
    u32 tags;

    ulong total_pages;
    ulong avail_pages;

    struct palloc_node buddies[PALLOC_ORDER_COUNT];

    spinlock_t lock;
};


/*
 * Stats
 */
struct palloc_page_stats {
    volatile u64 num_pages_total;
    volatile u64 num_pages_alloc;

    spinlock_t lock;
};

static struct palloc_page_stats page_stats = {
    .lock = SPINLOCK_INIT,
    .num_pages_total = 0,
    .num_pages_alloc = 0,
};

void palloc_stats_page(u64 *total, u64 *alloc)
{
    spinlock_exclusive_int(&page_stats.lock) {
        if (total) *total = page_stats.num_pages_total;
        if (alloc) *alloc = page_stats.num_pages_alloc;
    }
}


/*
 * Node manipulation
 */
static ppfn_t pfn_entry_count = 0;
static ppfn_t pfn_start = 0;
static ppfn_t pfn_limit = 0;

static int region_count = 0;
static struct palloc_region regions[MAX_PALLOC_REGION_COUNT];
static struct palloc_node *nodes;

static struct palloc_node *get_node_by_pfn(ppfn_t pfn)
{
    panic_if(pfn < pfn_start || pfn >= pfn_limit,
             "PFN %lx out of range: %lx to %lx\n", pfn, pfn_start, pfn_limit);

    return &nodes[pfn - pfn_start];
}

static void insert_node(ppfn_t pfn, int region, int order)
{
    struct palloc_node *node = get_node_by_pfn(pfn);

    node->next = regions[region].buddies[order].next;
    node->has_next = regions[region].buddies[order].has_next;

    regions[region].buddies[order].has_next = 1;
    regions[region].buddies[order].next = pfn;
}

static void remove_node(ppfn_t pfn, int region, int order)
{
    ppfn_t cur_pfn = regions[region].buddies[order].next;
    struct palloc_node *cur = get_node_by_pfn(cur_pfn);
    struct palloc_node *prev = NULL;

    // If the node is the first one
    if (cur_pfn == pfn) {
        regions[region].buddies[order].has_next = cur->has_next;
        regions[region].buddies[order].next = cur->next;

        cur->has_next = 0;
        cur->next = 0;

        return;
    }

    // Otherwise we have to go through the list
    assert(cur->has_next);

    do {
        prev = cur;
        cur_pfn = cur->next;
        cur = get_node_by_pfn(cur_pfn);

        if (cur_pfn == pfn) {
            prev->has_next = cur->has_next;
            prev->next = cur->next;

            cur->has_next = 0;
            cur->next = 0;

            return;
        }
    } while (cur->has_next);

    panic("Unable to remove node from list, PFN: %llx, region: %d, order: %d\n",
          (u64)pfn, region, order);
}


/*
 * Helper functions
 */
static ppfn_t pfn_mod_order_page_count(ppfn_t pfn, paddr_t order_page_count)
{
    return pfn & (order_page_count - 1);
}

int calc_palloc_order(int count)
{
    int order = 32 - 1 - clz32(count);
    if (popcount32(count) > 1) {
        order++;
    }

    if (order < PALLOC_MIN_ORDER) {
        order = PALLOC_MIN_ORDER;
    } else if (order > PALLOC_MAX_ORDER) {
        panic("Too many pages to allocate: %d", count);
        order = -1;
    }

    return order;
}

void buddy_print()
{
    for (int r = 0; r < region_count; r++) {
        struct palloc_region *region = &regions[r];
        kprintf("\tBuddies in region #%d with tag %x\n", r, region->tags);

        for (int order = PALLOC_MIN_ORDER; order <= PALLOC_MAX_ORDER; order++) {
            int count = 0;

            int has_next = region->buddies[order].has_next;
            struct palloc_node *cur = has_next ?
                        get_node_by_pfn(region->buddies[order].next) : NULL;

            while (has_next) {
                count++;

                has_next = cur->has_next;
                if (has_next) {
                    cur = get_node_by_pfn(cur->next);
                }
            }

            kprintf("\t\tOrder: %d, Count: %d\n", order, count);
        }
    }
}


/*
 * Buddy
 */
static int buddy_split(int order, int region)
{
    panic_if(order > PALLOC_MAX_ORDER || order < PALLOC_MIN_ORDER,
             "Invalid order!\n");

    // Split higher order buddies if necessary
    if (!regions[region].buddies[order].has_next) {
        // If this is the highest order, then fail
        if (order == PALLOC_MAX_ORDER) {
            kprintf("Unable to split buddy\n");
            return -1;
        }

        if (-1 == buddy_split(order + 1, region)) {
            return -1;
        }
    }

    // First obtain the palloc node
    ppfn_t pfn = regions[region].buddies[order].next;
    struct palloc_node *node = get_node_by_pfn(pfn);

    // Remove the node from the list
    regions[region].buddies[order].next = node->next;
    regions[region].buddies[order].has_next = node->has_next;

    // Obtain the other node
    ppfn_t pfn2 = pfn + ((ppfn_t)0x1 << (order - 1));
    struct palloc_node *node2 = get_node_by_pfn(pfn2);

    // Set up the two nodes
    node->alloc = 0;
    node->order = order - 1;
    node->region = region;
    node->avail = 1;
    node->has_next = 0;
    node->next = 0;

    node2->alloc = 0;
    node2->order = order - 1;
    node2->region = region;
    node2->avail = 1;
    node2->has_next = 0;
    node2->next = 0;

    // Insert the nodes into the lower order list
    insert_node(pfn, region, order - 1);
    insert_node(pfn2, region, order - 1);

    return 0;
}

static void buddy_combine(ppfn_t pfn)
{
    // Obtain the node
    struct palloc_node *node = get_node_by_pfn(pfn);
    int region = node->region;
    int order = node->order;
    ulong order_page_count = 0x1ul << order;

    // If we already have the highest order, then we are done
    if (order == PALLOC_MAX_ORDER) {
        return;
    }

    // Get some info of the higher order
    int higher = order + 1;
    ulong higher_order_count = 0x1ul << higher;
    ppfn_t higher_pfn = 0;
    ppfn_t other_pfn = 0;

    // Check the other node to see if they can form a buddy
    if (0 == pfn_mod_order_page_count(pfn, higher_order_count)) {
        higher_pfn = pfn;
        other_pfn = pfn + order_page_count;
    } else {
        higher_pfn = pfn - order_page_count;
        other_pfn = pfn - order_page_count;
    }
    struct palloc_node *other_node = get_node_by_pfn(other_pfn);

    // If the other node doesn't belong to the same region as current node,
    // or the other node is in use, then we are done,
    // as there's no need to combine further
    if (
        !other_node || !other_node->avail || other_node->region != region ||
        other_node->alloc || order != other_node->order
    ) {
        return;
    }

    // Remove both nodes from the list
    remove_node(pfn, region, order);
    remove_node(other_pfn, region, order);

    // Setup the two nodes
    node->has_next = 0;
    node->next = 0;
    node->order = higher;
    node->region = region;
    node->avail = 1;

    other_node->has_next = 0;
    other_node->next = 0;
    other_node->order = higher;
    other_node->region = region;
    other_node->avail = 1;

    // Insert them into the higher order list
    insert_node(higher_pfn, region, higher);

    // Combine the higiher order
    buddy_combine(higher_pfn);
}


/*
 * Alloc and free
 */
ppfn_t palloc_region(int count, int region)
{
    int order = calc_palloc_order(count);
    int order_count = 0x1 << order;
    //kprintf("order: %d, order_count: %d\n", order, order_count);

    // See if this region has enough pages to allocate
    if (regions[region].avail_pages < order_count) {
        return 0;
    }

    // If this is the highest order, then we are not able to allocate the pages
    if (order == PALLOC_MAX_ORDER) {
        return 0;
    }

    // Lock the region
    spinlock_lock_int(&regions[region].lock);

    // Split higher order buddies if necessary
    if (!regions[region].buddies[order].has_next) {
        if (-1 == buddy_split(order + 1, region)) {
            kprintf("Unable to split buddy");

            spinlock_unlock_int(&regions[region].lock);
            return 0;
        }
    }

    // Now we are safe to allocate, first obtain the palloc node
    ppfn_t pfn = regions[region].buddies[order].next;
    struct palloc_node *node = get_node_by_pfn(pfn);

    // Remove the node from the list
    regions[region].buddies[order].next = node->next;
    regions[region].buddies[order].has_next = node->has_next;

    // Mark the node as allocated
    node->alloc = 1;
    node->has_next = 0;
    node->next = 0;
    regions[region].avail_pages -= order_count;

    // Unlock the region
    spinlock_unlock_int(&regions[region].lock);

    // Update page stats
    spinlock_exclusive_int(&page_stats.lock) {
        page_stats.num_pages_alloc += order_count;
    }

    return pfn;
}

ppfn_t palloc_tag(int count, u32 mask, int match)
{
    for (int r = 0; r < region_count; r++) {
        struct palloc_region *region = &regions[r];

        int matched = 0;
        switch (match) {
        case MEMMAP_ALLOC_MATCH_IGNORE:
            matched = 1;
            break;
        case MEMMAP_ALLOC_MATCH_EXACT:
            matched = region->tags == mask ? 1 : 0;
            break;
        case MEMMAP_ALLOC_MATCH_SET_ALL:
            matched = (region->tags & mask) == mask ? 1 : 0;
            break;
        case MEMMAP_ALLOC_MATCH_SET_ANY:
            matched = region->tags & mask ? 1 : 0;
            break;
        case MEMMAP_ALLOC_MATCH_UNSET_ALL:
            matched = (~region->tags & mask) == mask ? 1 : 0;
            break;
        case MEMMAP_ALLOC_MATCH_UNSET_ANY:
            matched = ~region->tags & mask ? 1 : 0;
            break;
        default:
            break;
        }

        if (matched) {
            ppfn_t pfn = palloc_region(count, r);
            if (pfn) {
                return pfn;
            }
        }
    }

    return 0;
}

ppfn_t palloc_direct_mapped(int count)
{
    static int tags[] = {
        MEMMAP_ALLOC_MATCH_EXACT,   MEMMAP_TAG_NORMAL | MEMMAP_TAG_DIRECT_MAPPED | MEMMAP_TAG_DIRECT_ACCESS,
        MEMMAP_ALLOC_MATCH_SET_ALL, MEMMAP_TAG_NORMAL | MEMMAP_TAG_DIRECT_MAPPED | MEMMAP_TAG_DIRECT_ACCESS,
        MEMMAP_ALLOC_MATCH_EXACT,   MEMMAP_TAG_NORMAL | MEMMAP_TAG_DIRECT_MAPPED,
        MEMMAP_ALLOC_MATCH_SET_ALL, MEMMAP_TAG_NORMAL | MEMMAP_TAG_DIRECT_MAPPED,
    };

    ppfn_t pfn = 0;
    for (int t = 0; t < sizeof(tags) / sizeof(int) && !pfn; t += 2) {
        pfn = palloc_tag(count, tags[t + 1], tags[t]);
    }

    //kprintf("palloc direct count: %d, PPFN @ %llx, paddr @ %llx\n",
    //        count, (u64)pfn, (u64)ppfn_to_paddr(pfn));

    panic_if(!pfn, "Unable to allocate direct mapped physical memory!\n");
    return pfn;
}

ppfn_t palloc(int count)
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

    ppfn_t pfn = 0;
    for (int t = 0; t < sizeof(tags) / sizeof(int) && !pfn; t += 2) {
        warn_if(tags[t] == MEMMAP_ALLOC_MATCH_IGNORE,
                "Unable to find memory region with the proper tags set!\n");
        pfn = palloc_tag(count, tags[t + 1], tags[t]);
    }

    //kprintf("palloc PPFN @ %llx, paddr @ %llx\n", (u64)pfn, (u64)ppfn_to_paddr(pfn));

    return pfn;
}

int pfree(ppfn_t pfn)
{
    // Obtain the node
    struct palloc_node *node = get_node_by_pfn(pfn);

    // Get region and order
    int region = node->region;
    int order = node->order;
    int order_count = 0x1 << order;

    // Lock the region
    spinlock_lock_int(&regions[region].lock);

    // Setup the node
    node->alloc = 0;

    // Insert the node back to the list
    if (regions[region].buddies[order].value) {
        node->next = regions[region].buddies[order].next;
        node->has_next = 1;
    } else {
        node->next = 0;
        node->has_next = 0;
    }
    regions[region].buddies[order].has_next = 1;
    regions[region].buddies[order].next = pfn;

    // Setup the region
    regions[region].avail_pages += order_count;

    // Combine the buddy system
    buddy_combine(pfn);

    // Unlock the region
    spinlock_unlock_int(&regions[region].lock);

    // Update page stats
    spinlock_exclusive_int(&page_stats.lock) {
        page_stats.num_pages_alloc -= order_count;
    }

    return order_count;
}

paddr_t palloc_paddr(int count)
{
    ppfn_t ppfn = palloc(count);
    paddr_t paddr = ppfn_to_paddr(ppfn);
    return paddr;
}

paddr_t palloc_paddr_direct_mapped(int count)
{
    ppfn_t ppfn = palloc_direct_mapped(count);
    paddr_t paddr = ppfn_to_paddr(ppfn);
    return paddr;
}

int pfree_paddr(paddr_t paddr)
{
    ppfn_t ppfn = paddr_to_ppfn(paddr);
    return pfree(ppfn);
}

void *palloc_ptr(int count)
{
    paddr_t paddr = palloc_paddr_direct_mapped(count);
    return paddr ? hal_cast_paddr_to_kernel_ptr(paddr) : NULL;
}

int pfree_ptr(void *ptr)
{
    paddr_t paddr = hal_cast_kernel_ptr_to_paddr(ptr);
    ppfn_t ppfn = paddr_to_ppfn(paddr);
    return pfree(ppfn);
}


/*
 * Initialization
 */
void reserve_palloc()
{
    // Calculate total number of nodes - 1 page needs 1 node
    pfn_entry_count = get_pfn_range(&pfn_start, &pfn_limit);

    ulong node_bytes = pfn_entry_count * sizeof(struct palloc_node);
    ulong aligned_node_bytes = align_up_vsize(node_bytes, PAGE_SIZE);

    kprintf("\tPalloc node size: %ld KB, num pages: %ld\n",
            aligned_node_bytes / 1024, aligned_node_bytes / PAGE_SIZE);

    // Allocate and reserve memory
    u64 nodes_u64 = find_free_memmap_direct_mapped_region(aligned_node_bytes, PAGE_SIZE);
    paddr_t nodes_paddr = cast_u64_to_paddr(nodes_u64);
    nodes = hal_cast_paddr_to_kernel_ptr(nodes_paddr);

    // Initialize all nodes
    memzero(nodes, aligned_node_bytes);

    kprintf("\tMemory reserved for palloc nodes @ %llx, size: %llx bytes\n",
            (u64)nodes_u64, (u64)aligned_node_bytes);
}

static void init_region(ppfn_t start_pfn, ulong count, int region, u32 tags)
{
    kprintf("\tInitializing region, start PFN: %llx, len: %lx, region: %d\n",
            (u64)start_pfn, count, region);

    // Update page stats
    page_stats.num_pages_total += count;

    // Update page count
    regions[region].tags = tags;
    regions[region].total_pages += count;
    regions[region].avail_pages += count;

    // Setup the buddy system
    int order;

    ppfn_t cur_pfn = start_pfn;
    ppfn_t limit = start_pfn + count;

    while (cur_pfn < limit) {
        for (order = PALLOC_MAX_ORDER; order >= PALLOC_MIN_ORDER; order--) {
            ulong order_page_count = 0x1ul << order;

            // We found the correct buddy size
            if (
                pfn_mod_order_page_count(cur_pfn, order_page_count) == 0 &&
                cur_pfn + order_page_count <= limit
            ) {
                // Obtain the node
                struct palloc_node *node = get_node_by_pfn(cur_pfn);
                node->order = order;
                node->alloc = 0;
                node->region = region;
                node->avail = 1;
                node->next = 0;
                node->has_next = 0;

                // Insert the chunk into the buddy list
                insert_node(cur_pfn, region, order);
                kprintf("\t\tBuddy inserted, PFN: %llx, paddr @ %llx, #pages: %lx, order: %d\n",
                        (u64)cur_pfn, (u64)ppfn_to_paddr(cur_pfn), order_page_count, order);

                cur_pfn += order_page_count;
                break;
            }
        }
    }
}

void init_palloc()
{
    kprintf("Initializing page frame allocator\n");

    // Initialize region list
    int j, k;
    for (j = 0; j < MAX_PALLOC_REGION_COUNT; j++) {
        regions[j].tags = 0;
        regions[j].total_pages = 0;
        regions[j].avail_pages = 0;

        for (k = 0; k < PALLOC_ORDER_COUNT; k++) {
            regions[j].buddies[k].value = 0;
        }

        spinlock_init(&regions[j].lock);
    }

    // Go through PFN database to construct regions
    u32 cur_tag = 0;
    int recording = 0;
    ulong cur_region_start = 0;

    for (ulong i = pfn_start; i < pfn_start + pfn_entry_count; i++) {
        struct pfndb_entry *entry = get_pfn_entry_by_pfn(i);

        // End of a region
        if (recording &&
            (!entry->usable || entry->inuse || cur_tag != entry->tags)
        ) {
            kprintf("cur region start: %x, i: %x\n", cur_region_start, i);
            recording = 0;

            init_region(cur_region_start, i - cur_region_start, region_count, cur_tag);
            region_count++;
        }

        // Start of a region
        else if (!recording &&
            (entry->usable && !entry->inuse)
        ) {
            cur_region_start = i;
            cur_tag = entry->tags;
            recording = 1;
        }
    }

    // Take care of the last region
    if (recording) {
        kprintf("last region start: %x, i: %x\n", cur_region_start, pfn_limit);
        init_region(cur_region_start, pfn_limit - cur_region_start, region_count, cur_tag);
        region_count++;
    }

    // Print the buddy
    buddy_print();
}


/*
 * Testing
 */
#define PALLOC_TEST_ORDER_COUNT 8
#define PALLOC_TEST_PER_ORDER   10
#define PALLOC_TEST_LOOPS       5

void test_palloc()
{
    kprintf("Testing palloc\n");

    int i, j, k;
    ulong results[PALLOC_TEST_ORDER_COUNT][PALLOC_TEST_PER_ORDER];

    for (k = 0; k < PALLOC_TEST_LOOPS; k++) {


        for (i = 0; i < PALLOC_TEST_ORDER_COUNT; i++) {
            for (j = 0; j < PALLOC_TEST_PER_ORDER; j++) {
                int count = 0x1 << i;
                //kprintf("Allocating count: %d, index: %d", count, j);
                ulong pfn = palloc(count);
                results[i][j] = pfn;
                //kprintf(", PFN: %p\n", pfn);
            }
        }

        for (i = 0; i < PALLOC_TEST_ORDER_COUNT; i++) {
            for (j = 0; j < PALLOC_TEST_PER_ORDER; j++) {
                ulong pfn = results[i][j];
                if (pfn) {
                    //int count = 0x1 << i;
                    //kprintf("Freeing count: %d, index: %d, PFN: %p\n", count, j, pfn);
                    pfree(pfn);
                }
            }
        }
    }

    //buddy_print();

    kprintf("Successfully passed the test!\n");
}
