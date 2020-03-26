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


#define PALLOC_ORDER_BITS       4
#define PALLOC_MAX_ORDER        ((1 << PALLOC_ORDER_BITS) - 1)
#define PALLOC_MIN_ORDER        0
#define PALLOC_ORDER_COUNT      (PALLOC_MAX_ORDER - PALLOC_MIN_ORDER + 1)

#define PALLOC_REGION_BITS      4
#define PALLOC_REGION_COUNT     (1 << PALLOC_REGION_BITS)
#define PALLOC_DEFAULT_REGION   0
#define PALLOC_DUMMY_REGION     (PALLOC_REGION_COUNT - 1)

#define PFN_BITS                (sizeof(ulong) * 8 - PAGE_BITS)


struct palloc_node {
    union {
        ulong value;
        struct {
            ulong order     : PALLOC_ORDER_BITS;
            ulong tag       : PALLOC_REGION_BITS;
            ulong alloc     : 1;
            ulong has_next  : 1;
            ulong avail     : 1;
            ulong next      : PFN_BITS;
        };
    };
} packed_struct;

struct palloc_region {
    int region_tag;

    ulong total_pages;
    ulong avail_pages;

    struct palloc_node buddies[PALLOC_ORDER_COUNT];

    spinlock_t lock;
};


/*
 * Node manipulation
 */
static ulong pfn_start = 0;
static ulong pfn_limit = 0;

static struct palloc_node *nodes;
static struct palloc_region regions[PALLOC_REGION_COUNT];

// static struct palloc_node *get_node_by_paddr(ulong paddr)
// {
//     return &nodes[ADDR_TO_PFN(paddr)];
// }

static struct palloc_node *get_node_by_pfn(ulong pfn)
{
    panic_if(pfn < pfn_start || pfn >= pfn_limit,
             "PFN %lx out of range: %lx to %lx\n", pfn, pfn_start, pfn_limit);

    return &nodes[pfn - pfn_start];
}

static void insert_node(ulong pfn, int tag, int order)
{
    struct palloc_node *node = get_node_by_pfn(pfn);

    node->next = regions[tag].buddies[order].next;
    node->has_next = regions[tag].buddies[order].has_next;

    regions[tag].buddies[order].has_next = 1;
    regions[tag].buddies[order].next = pfn;
}

static void remove_node(ulong pfn, int tag, int order)
{
    ulong cur_pfn = regions[tag].buddies[order].next;
    struct palloc_node *cur = get_node_by_pfn(cur_pfn);
    struct palloc_node *prev = NULL;

    // If the node is the first one
    if (cur_pfn == pfn) {
        regions[tag].buddies[order].has_next = cur->has_next;
        regions[tag].buddies[order].next = cur->next;

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

    panic("Unable to remove node from list, PFN: %p, tag: %d, order: %d\n",
          pfn, tag, order);
}


/*
 * Helper functions
 */
static ulong pfn_mod_order_page_count(ulong pfn, ulong order_page_count)
{
    return pfn & (order_page_count - 1);
}

static int calc_palloc_order(int count)
{
    int order = 0;

    switch (popcount(count)) {
    case 0:
        order = 0;
        break;
    case 1:
        order = count;
        break;
    default:
        order = 1 << (32 - clz32(count));
        break;
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
    for (int tag = 0; tag < PALLOC_REGION_COUNT; tag++) {
        if (
            tag == PALLOC_DUMMY_REGION ||
            0 == regions[tag].avail_pages
        ) {
            continue;
        }

        kprintf("\tBuddy in Bucket #%d\n", tag);

        for (int order = PALLOC_MIN_ORDER; order <= PALLOC_MAX_ORDER; order++) {
            int count = 0;

            int has_next = regions[tag].buddies[order].has_next;
            struct palloc_node *cur = get_node_by_pfn(regions[tag].buddies[order].next);

            while (has_next) {
                count++;

                has_next = cur->has_next;
                cur = get_node_by_pfn(cur->next);
            }

            kprintf("\t\tOrder: %d, Count: %d\n", order, count);
        }
    }
}


/*
 * Buddy
 */
static int buddy_split(int order, int tag)
{
    assert(tag < PALLOC_REGION_COUNT && tag != PALLOC_DUMMY_REGION);
    assert(order <= PALLOC_MAX_ORDER && order > PALLOC_MIN_ORDER);

    // Split higher order buddies if necessary
    if (!regions[tag].buddies[order].has_next) {
        // If this is the highest order, then fail
        if (order == PALLOC_MAX_ORDER) {
            kprintf("Unable to split buddy\n");
            return -1;
        }

        if (-1 == buddy_split(order + 1, tag)) {
            return -1;
        }
    }

    // First obtain the palloc node
    ulong pfn = regions[tag].buddies[order].next;
    struct palloc_node *node = get_node_by_pfn(pfn);

    // Remove the node from the list
    regions[tag].buddies[order].next = node->next;
    regions[tag].buddies[order].has_next = node->has_next;

    // Obtain the other node
    ulong pfn2 = pfn + ((ulong)0x1 << (order - 1));
    struct palloc_node *node2 = get_node_by_pfn(pfn2);

    // Set up the two nodes
    node->alloc = 0;
    node->order = order - 1;
    node->tag = tag;
    node->avail = 1;
    node->has_next = 0;
    node->next = 0;

    node2->alloc = 0;
    node2->order = order - 1;
    node2->tag = tag;
    node2->avail = 1;
    node2->has_next = 0;
    node2->next = 0;

    // Insert the nodes into the lower order list
    insert_node(pfn, tag, order - 1);
    insert_node(pfn2, tag, order - 1);

    return 0;
}

static void buddy_combine(ulong pfn)
{
    // Obtain the node
    struct palloc_node *node = get_node_by_pfn(pfn);
    int tag = node->tag;
    int order = node->order;
    ulong order_page_count = 0x1ul << order;

    // If we already have the highest order, then we are done
    if (order == PALLOC_MAX_ORDER) {
        return;
    }

    // Get some info of the higher order
    int higher = order + 1;
    ulong higher_order_count = 0x1ul << higher;
    ulong higher_pfn = 0;
    ulong other_pfn = 0;
    ulong cur_addr = PFN_TO_ADDR(pfn);

    // Check the other node to see if they can form a buddy
    if (0 == pfn_mod_order_page_count(cur_addr, higher_order_count)) {
        higher_pfn = pfn;
        other_pfn = pfn + order_page_count;
    } else {
        higher_pfn = pfn - order_page_count;
        other_pfn = pfn - order_page_count;
    }
    struct palloc_node *other_node = get_node_by_pfn(other_pfn);

    // If the other node doesn't belong to the same region as current node,
    // or the other node is in use, then we are done, there's no way to combine
    if (
        !other_node || !other_node->avail || other_node->tag != tag ||
        other_node->alloc || order != other_node->order
    ) {
        return;
    }

    // Remove both nodes from the list
    remove_node(pfn, tag, order);
    remove_node(other_pfn, tag, order);

    // Setup the two nodes
    node->has_next = 0;
    node->next = 0;
    node->order = higher;
    node->tag = tag;
    node->avail = 1;
    other_node->has_next = 0;
    other_node->next = 0;
    other_node->order = higher;
    other_node->tag = tag;
    other_node->avail = 1;

    // Insert them into the higher order list
    insert_node(higher_pfn, tag, higher);

    // Combine the higiher order
    buddy_combine(higher_pfn);
}


/*
 * Alloc and free
 */
ulong palloc_tag(int count, int tag)
{
    assert(tag < PALLOC_REGION_COUNT && tag != PALLOC_DUMMY_REGION);
    int order = calc_palloc_order(count);
    int order_count = 0x1 << order;

    // See if this region has enough pages to allocate
    if (regions[tag].avail_pages < order_count) {
        return -1;
    }

    // If this is the highest order, then we are not able to allocate the pages
    if (order == PALLOC_MAX_ORDER) {
        return -1;
    }

    // Lock the region
    spinlock_lock_int(&regions[tag].lock);

    // Split higher order buddies if necessary
    if (!regions[tag].buddies[order].has_next) {
        if (-1 == buddy_split(order + 1, tag)) {
            kprintf("Unable to split buddy");

            spinlock_unlock_int(&regions[tag].lock);
            return -1;
        }
    }

    // Now we are safe to allocate, first obtain the palloc node
    ulong pfn = regions[tag].buddies[order].next;
    struct palloc_node *node = get_node_by_pfn(pfn);

    // Remove the node from the list
    regions[tag].buddies[order].next = node->next;
    regions[tag].buddies[order].has_next = node->has_next;

    // Mark the node as allocated
    node->alloc = 1;
    node->has_next = 0;
    node->next = 0;
    regions[tag].avail_pages -= order_count;

    // Unlock the region
    spinlock_unlock_int(&regions[tag].lock);

    return pfn;
}

ulong palloc(int count)
{
    ulong result = palloc_tag(count, PALLOC_DEFAULT_REGION);
    if (result) {
        return result;
    }

    int i;
    for (i = 0; i < PALLOC_REGION_COUNT; i++) {
        if (i != PALLOC_DEFAULT_REGION && i != PALLOC_DUMMY_REGION) {
            result = palloc_tag(count, PALLOC_DEFAULT_REGION);
            if (result) {
                return result;
            }
        }
    }

    return 0;
}

int pfree(ulong pfn)
{
    // Obtain the node
    struct palloc_node *node = get_node_by_pfn(pfn);

    // Get tag and order
    int tag = node->tag;
    int order = node->order;
    int order_count = 0x1 << order;

    // Setup the node
    node->alloc = 0;

    // Lock the region
    spinlock_lock_int(&regions[tag].lock);

    // Insert the node back to the list
    if (regions[tag].buddies[order].value) {
        node->next = regions[tag].buddies[order].next;
        node->has_next = 1;
    } else {
        node->next = 0;
        node->has_next = 0;
    }
    regions[tag].buddies[order].has_next = 1;
    regions[tag].buddies[order].next = pfn;

    // Setup the region
    regions[tag].avail_pages += order_count;

    // Combine the buddy system
    buddy_combine(pfn);

    // Unlock the region
    spinlock_unlock_int(&regions[tag].lock);

    return order_count;
}


/*
 * Initialization
 */
static void init_region(ulong start_pfn, ulong count, int tag)
{
    kprintf("\tInitializing region, start PFN: %lx, len: %lx, tag: %d\n",
            start_pfn, count, tag);

    assert(tag < PALLOC_REGION_COUNT && tag != PALLOC_DUMMY_REGION);

    // Update page count
    regions[tag].total_pages += count;
    regions[tag].avail_pages += count;

    // Setup the buddy system
    int order;

    ulong cur_pfn = start_pfn;
    ulong limit = start_pfn + count;

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
                node->tag = tag;
                node->avail = 1;
                node->next = 0;
                node->has_next = 0;

                // Insert the chunk into the buddy list
                insert_node(cur_pfn, tag, order);
                kprintf("\t\tBuddy inserted, PFN: %lx, #pages: %lx, order: %d\n",
                        cur_pfn, order_page_count, order);

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
    for (j = 0; j < PALLOC_REGION_COUNT; j++) {
        regions[j].region_tag = j;
        regions[j].total_pages = 0;
        regions[j].avail_pages = 0;

        for (k = 0; k < PALLOC_ORDER_COUNT; k++) {
            regions[j].buddies[k].value = 0;
        }

        spinlock_init(&regions[j].lock);
    }

    // Calculate total number of nodes - 1 page needs 1 node
    ulong pfn_entry_count = get_pfn_range(&pfn_start);
    pfn_limit = pfn_start + pfn_entry_count;

    ulong node_size = pfn_entry_count * sizeof(struct palloc_node);
    kprintf("\tPalloc node size: %d KB, pages: %d\n",
            node_size / 1024, node_size / PAGE_SIZE);

    // Allocate and reserve memory
    nodes = (void *)reserve_free_mem(node_size);

    // Initialize all nodes
    for (ulong i = 0; i < pfn_entry_count; i++) {
        nodes[i].avail = 0;
        nodes[i].tag = PALLOC_DUMMY_REGION;
    }

    // Go through PFN database to construct tags array
    int cur_tag = -1;
    int recording = 0;
    ulong cur_region_start = 0;

    for (ulong i = pfn_start; i < pfn_start + pfn_entry_count; i++) {
        struct pfndb_entry *entry = get_pfn_entry_by_pfn(i);

        // End of a region
        if (
            recording &&
            (!entry->usable || entry->inuse || cur_tag != entry->tag)
        ) {
            kprintf("cur region start: %x, i: %x\n", cur_region_start, i);
            init_region(cur_region_start, i - cur_region_start, cur_tag);
            recording = 0;
        }

        // Start of a region
        else if (
            !recording &&
            (entry->usable && !entry->inuse && entry->tag != PALLOC_DUMMY_REGION)
        ) {
            cur_region_start = i;
            cur_tag = entry->tag;
            recording = 1;
        }
    }

    // Take care of the last region
    if (recording) {
        kprintf("last region start: %x, i: %x\n", cur_region_start, pfn_limit);
        init_region(cur_region_start, pfn_limit - cur_region_start, cur_tag);
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
