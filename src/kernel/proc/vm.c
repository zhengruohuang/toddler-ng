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
static salloc_obj_t vm_salloc_obj;

void init_vm()
{
    kprintf("Initializing dynamic VM allocator\n");
    salloc_create(&vm_salloc_obj, sizeof(struct vm_block), 0, 0, NULL, NULL);
}


/*
 * Sort compare
 */
static int list_compare(list_node_t *a, list_node_t *b)
{
    struct vm_block *ba = list_entry(a, struct vm_block, node);
    struct vm_block *bb = list_entry(b, struct vm_block, node);
    if (ba->base > bb->base) {
        return 1;
    } else if (ba->base < bb->base) {
        return -1;
    } else {
        return 0;
    }
}


/*
 * Create
 */
int vm_create(struct process *p)
{
    struct vm_block *b = salloc(&vm_salloc_obj);
    panic_if(!b, "Unable to allocate VM block!\n");

    b->proc = p;
    b->base = p->vm.dynamic.start;
    b->size = p->vm.dynamic.end - p->vm.dynamic.start;
    b->type = VM_TYPE_AVAIL;

    list_push_back_exclusive(&p->vm.avail_unmapped, &b->node);
    ref_count_inc(&p->ref_count);

    return 0;
}


/*
 * Allocate
 */
struct vm_block *vm_alloc_thread(struct process *p, struct thread *t, vm_block_thread_mapper_t mapper)
{
    // Try reuse first
    list_node_t *n = list_pop_back_exclusive(&p->vm.reuse_mapped);
    if (n) {
        struct vm_block *b = list_entry(n, struct vm_block, node);
        panic_if(b->size != t->memory.block_size,
                 "Thread VM block size mismatch, b->size: %lu, t->memory.block_size: %lu!\n",
                 b->size, t->memory.block_size);

        list_insert_sorted_exclusive(&p->vm.inuse_mapped, &b->node, list_compare);
        return b;
    }

    // Allocate a new block
    struct vm_block *b = salloc(&vm_salloc_obj);
    panic_if(!b, "Unable to allocate VM block!\n");

    ulong base = 0;
    struct vm_block *delete_block = NULL;

    list_access_exclusive(&p->vm.avail_unmapped) {
        list_node_t *n = list_back(&p->vm.avail_unmapped);
        if (!n) {
            break;
        }

        struct vm_block *last = list_entry(n, struct vm_block, node);
        if (last->size < t->memory.block_size) {
            break;
        }

        last->size -= t->memory.block_size;
        base = last->base + last->size;

        if (!last->size) {
            delete_block = last;
            list_remove(&p->vm.avail_unmapped, &last->node);
        }
    }

    if (base) {
        b->proc = p;
        b->base = base;
        b->size = t->memory.block_size;
        b->type = VM_TYPE_THREAD;

        if (mapper) {
            mapper(p, t, b);
        }

        ref_count_inc(&p->ref_count);
        list_insert_sorted_exclusive(&p->vm.inuse_mapped, &b->node, list_compare);
    } else {
        sfree(b);
        b = NULL;
    }

    if (delete_block) {
        sfree(delete_block);
        delete_block = NULL;
        ref_count_dec(&p->ref_count);
    }

    return b;
}

struct vm_block *vm_alloc(struct process *p, ulong base, ulong size, ulong attri)
{
    ulong end = align_up_vaddr(base + size, PAGE_SIZE);
    base = align_down_vsize(base, PAGE_SIZE);
    size = end - base;

    struct vm_block *b = salloc(&vm_salloc_obj);
    panic_if(!b, "Unable to allocate VM block!\n");

    struct vm_block *extra_b = salloc(&vm_salloc_obj);
    panic_if(!b, "Unable to allocate VM block!\n");

    int found = 0, extra_used = 0;
    struct vm_block *delete_block = NULL;

    list_access_exclusive(&p->vm.avail_unmapped) {
        struct vm_block *target_block = NULL;
        list_foreach(&p->vm.avail_unmapped, n) {
            struct vm_block *cur_block = list_entry(n, struct vm_block, node);
            if (base) {
                ulong cur_end = cur_block->base + cur_block->size;
                if (cur_block->base <= base && cur_end >= end) {
                    target_block = cur_block;
                    break;
                }
            } else if (cur_block->size >= size) {
                target_block = cur_block;
                break;
            }
        }

        if (!target_block) {
            break;
        }

        found = 1;
        if (base) {
            b->base = base;

            ulong target_end = target_block->base + target_block->size;
            if (target_block->base == base) {
                if (target_end == end) {
                    delete_block = target_block;
                    list_remove(&p->vm.avail_unmapped, &target_block->node);
                } else {
                    target_block->base += size;
                }
            } else if (target_end == end) {
                target_block->size -= size;
            } else {
                target_block->size = base - target_block->base;
                extra_b->base = end;
                extra_b->size = target_end - end;

                ref_count_inc(&p->ref_count);
                list_insert(&p->vm.avail_unmapped, &target_block->node, &extra_b->node);
                extra_used = 1;
            }
        } else {
            b->base = target_block->base;

            target_block->size -= size;
            target_block->base += size;

            if (!target_block->size) {
                delete_block = target_block;
                list_remove(&p->vm.avail_unmapped, &target_block->node);
            }
        }
    }

    if (found) {
        b->proc = p;
        b->size = size;
        b->type = VM_TYPE_GENERIC;

        ref_count_inc(&p->ref_count);
        list_insert_sorted_exclusive(&p->vm.inuse_mapped, &b->node, list_compare);
    } else {
        sfree(b);
        b = NULL;
    }

    if (delete_block) {
        sfree(delete_block);
        delete_block = NULL;
        ref_count_dec(&p->ref_count);
    }
    if (!extra_used) {
        sfree(extra_b);
        extra_b = NULL;
    }

    return b;
}


/*
 * Free
 */
int vm_free_block(struct process *p, struct vm_block *b)
{
    struct vm_block *b_sanit = NULL;
    list_remove_exclusive(&p->vm.inuse_mapped, &b->node);

    if (b->type == VM_TYPE_THREAD) {
        list_access_exclusive(&p->vm.reuse_mapped) {
            list_insert_sorted(&p->vm.reuse_mapped, &b->node, list_compare);
            if (p->vm.reuse_mapped.count > hal_get_num_cpus()) {
                list_node_t *n = list_pop_front(&p->vm.reuse_mapped);
                b_sanit = list_entry(n, struct vm_block, node);
            }
        }
    } else {
        b_sanit = b;
    }

    if (b_sanit) {
        list_insert_sorted_exclusive(&p->vm.sanit_mapped, &b_sanit->node, list_compare);
        ulong wait_acks = request_tlb_shootdown(p, b_sanit);
        if (!wait_acks) {
            // TODO: merge
        }
    }

    return 0;
}

int vm_free(struct process *p, ulong base)
{
    struct vm_block *b_free = NULL;

    list_foreach_exclusive(&p->vm.inuse_mapped, n) {
        struct vm_block *b = list_entry(n, struct vm_block, node);
        if (b->base == base) {
            b_free = b;
            break;
        }
    }

    return b_free ? vm_free_block(p, b_free) : -1;
}

void vm_move_to_sanit_unmapped(struct process *p, struct vm_block *b)
{
    list_remove_exclusive(&p->vm.sanit_mapped, &b->node);
    list_insert_sorted_exclusive(&p->vm.sanit_unmapped, &b->node, list_compare);
}


/*
 * Map
 */
int vm_map(struct process *p, ulong addr, ulong prot)
{
    int err = -1;

    list_foreach_exclusive(&p->vm.inuse_mapped, n) {
        struct vm_block *b = list_entry(n, struct vm_block, node);
        ulong end = b->base + b->size;
        if (addr >= b->base && addr < end) {
            ulong page_base = align_down_ulong(addr, PAGE_SIZE);
            paddr_t paddr = palloc_paddr(1);
            get_hal_exports()->map_range(p->page_table,
                                         page_base,
                                         paddr,
                                         PAGE_SIZE,
                                         1, 1, 1, 0, 0);
            err = 0;
            break;
        }
    }

    return err;
}
