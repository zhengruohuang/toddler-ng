/*
 * Thread dynamic area allocator
 */


#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "kernel/include/kernel.h"
#include "kernel/include/lib.h"
#include "kernel/include/mem.h"
#include "kernel/include/atomic.h"
#include "kernel/include/proc.h"


/*
 * Init
 */
static salloc_obj_t dalloc_salloc_obj;

void init_dynamic_vm()
{
    kprintf("Initializing dynamic VM allocator\n");

    // Create salloc obj
    salloc_create(&dalloc_salloc_obj, sizeof(struct dynamic_block), 0, 0, NULL, NULL);
}


/*
 * Sort compare
 */
static int blocks_list_compare(list_node_t *a, list_node_t *b)
{
    struct dynamic_block *ba = list_entry(a, struct dynamic_block, node);
    struct dynamic_block *bb = list_entry(b, struct dynamic_block, node);
    if (ba->base > bb->base) {
        return 1;
    } else if (ba->base < bb->base) {
        return -1;
    } else {
        return 0;
    }
}

static int free_list_compare(list_node_t *a, list_node_t *b)
{
    struct dynamic_block *ba = list_entry(a, struct dynamic_block, node_free);
    struct dynamic_block *bb = list_entry(b, struct dynamic_block, node_free);
    if (ba->base > bb->base) {
        return 1;
    } else if (ba->base < bb->base) {
        return -1;
    } else {
        return 0;
    }
}

static int mapped_list_compare(list_node_t *a, list_node_t *b)
{
    struct dynamic_block *ba = list_entry(a, struct dynamic_block, node_mapped);
    struct dynamic_block *bb = list_entry(b, struct dynamic_block, node_mapped);
    if (ba->base > bb->base) {
        return 1;
    } else if (ba->base < bb->base) {
        return -1;
    } else {
        return 0;
    }
}


/*
 * Alloc
 */
static struct dynamic_block *alloc_new_block(struct process *p, ulong size)
{
    if (p->vm.dynamic.bottom - size <= p->vm.heap.end + PAGE_SIZE) {
        return NULL;
    }

    struct dynamic_block *block = (struct dynamic_block *)salloc(&dalloc_salloc_obj);

    p->vm.dynamic.bottom -= size;
    block->base = p->vm.dynamic.bottom;
    block->size = size;
    block->state = VM_STATE_INUSE;

    list_insert_sorted(&p->vm.dynamic.blocks, &block->node, blocks_list_compare);
    return block;
}

ulong vm_alloc(struct process *p, ulong size, ulong align, ulong attri)
{
    panic_if(align != PAGE_SIZE, "align must be PAGE_SIZE!\n");
    panic_if(size % PAGE_SIZE, "size must be multiple of PAGE_SIZE!\n");

    struct dynamic_block *block = NULL;

    // Find a block from free unmapped list
    list_node_t *n = list_pop_back(&p->vm.dynamic.free);
    if (n) {
        block = list_entry(n, struct dynamic_block, node_free);
    } else {
        block = alloc_new_block(p, size);
    }

    return block ? block->base : 0;
}


/*
 * Free
 */
static struct dynamic_block *find_inuse_block(struct process *p, ulong base)
{
    struct dynamic_block *block = NULL;

    list_foreach(&p->vm.dynamic.blocks, n) {
        struct dynamic_block *b = list_entry(n, struct dynamic_block, node);
        if (b->base == base) {
            panic_if(b->state != VM_STATE_INUSE, "Inconsistent VM block state!\n");
            block = b;
            break;
        }
    }

    return block;
}

void vm_free(struct process *p, ulong base)
{
    // FIXME: Not impelemented
    return;

//     struct dynamic_block *block = find_inuse_block(p, base);
//     panic_if(!block, "Unable to find VM block @ %lx\n", base);
//
//     block->state = VM_STATE_FREE_MAPPED;
//     list_insert_sorted(&p->vm.dynamic.mapped, &block->node_mapped, mapped_list_compare);
//
//     vm_cleanup(p);
}


/*
 * Clean up
 */
void vm_cleanup(struct process *p)
{
}


/*
 * Purge
 */
void vm_purge(struct process *p)
{
}
