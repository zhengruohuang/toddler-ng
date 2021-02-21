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

// static void vm_salloc_ctor(void *entry)
// {
//     memzero(entry, sizeof(struct vm_block));
// }

void init_vm()
{
    kprintf("Initializing dynamic VM allocator\n");
    salloc_create_default(&vm_salloc_obj, "vm", sizeof(struct vm_block));
}


/*
 * Sort ops
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

static int avail_list_merge(list_node_t *a, list_node_t *b)
{
    struct vm_block *ba = list_entry(a, struct vm_block, node);
    struct vm_block *bb = list_entry(b, struct vm_block, node);

    if (ba->base + ba->size == bb->base) {
        ba->size += bb->size;
        return 0;
    } else {
        return -1;
    }
}

static void avail_list_free(list_node_t *n)
{
    struct vm_block *b = list_entry(n, struct vm_block, node);
    ref_count_dec(&b->proc->ref_count);
    sfree_audit(b, &b->proc->vm.num_salloc_objs);
}


/*
 * Create
 */
int vm_create(struct process *p)
{
    struct vm_block *b = salloc_audit(&vm_salloc_obj, &p->vm.num_salloc_objs);
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
struct vm_block *vm_alloc_thread(struct process *p, struct thread *t,
                                 vm_block_thread_mapper_t mapper,
                                 vm_block_thread_mapper_t reuser)
{
    // Try reuse first
    list_node_t *n = list_pop_back_exclusive(&p->vm.reuse_mapped);
    if (n) {
        struct vm_block *b = list_entry(n, struct vm_block, node);
        panic_if(b->size != t->memory.block_size,
                 "Thread VM block size mismatch, b->size: %lu, t->memory.block_size: %lu!\n",
                 b->size, t->memory.block_size);

        reuser(p, t, b);
        list_insert_sorted_exclusive(&p->vm.inuse_mapped, &b->node, list_compare);
        return b;
    }

    // Allocate a new block
    struct vm_block *b = salloc_audit(&vm_salloc_obj, &p->vm.num_salloc_objs);
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

        // Increament ref count so that process cleaner won't miss this block
        ref_count_inc(&p->vm.num_active_blocks);
        ref_count_inc(&p->ref_count);
    }

    // FIXME: move this inside ``list_access_exclusive(&p->vm.avail_unmapped)''?
    if (base) {
        b->proc = p;
        b->base = base;
        b->size = t->memory.block_size;
        b->type = VM_TYPE_THREAD;
        b->map_type = VM_MAP_TYPE_OWNER;

        if (mapper) {
            mapper(p, t, b);
        }

        list_insert_sorted_exclusive(&p->vm.inuse_mapped, &b->node, list_compare);
    } else {
        sfree_audit(b, &p->vm.num_salloc_objs);
        b = NULL;
    }

    if (delete_block) {
        sfree_audit(delete_block, &p->vm.num_salloc_objs);
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

    struct vm_block *b = salloc_audit(&vm_salloc_obj, &p->vm.num_salloc_objs);
    panic_if(!b, "Unable to allocate VM block!\n");

    struct vm_block *extra_b = salloc_audit(&vm_salloc_obj, &p->vm.num_salloc_objs);
    panic_if(!extra_b, "Unable to allocate VM block!\n");

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

        //kprintf("target_block @ %p\n", target_block);

        if (!target_block) {
            break;
        }

        found = 1;

        // Increament ref count so that the process cleaner won't miss this block
        ref_count_inc(&p->vm.num_active_blocks);
        ref_count_inc(&p->ref_count);

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
                extra_b->proc = p;
                extra_b->base = end;
                extra_b->size = target_end - end;
                extra_b->type = VM_TYPE_AVAIL;

                // FIXME: is ref_count_inc really necessary?
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
        b->map_type = VM_MAP_TYPE_NONE;

        list_insert_sorted_exclusive(&p->vm.inuse_mapped, &b->node, list_compare);
    } else {
        sfree_audit(b, &p->vm.num_salloc_objs);
        b = NULL;
    }

    if (delete_block) {
        sfree_audit(delete_block, &p->vm.num_salloc_objs);
        delete_block = NULL;
        ref_count_dec(&p->ref_count);
    }
    if (!extra_used) {
        sfree_audit(extra_b, &p->vm.num_salloc_objs);
        extra_b = NULL;
    }

    return b;
}


/*
 * Free VM block
 */
static int _vm_free_removed_block(struct process *p, struct vm_block *b)
{
    struct vm_block *b_sanit = NULL;

    if (b->type == VM_TYPE_THREAD) {
        // TODO: use num cpus when debugged
        const int sanit_threshold = 0;  //hal_get_num_cpus();

        list_access_exclusive(&p->vm.reuse_mapped) {
            list_insert_sorted(&p->vm.reuse_mapped, &b->node, list_compare);
            if (p->vm.reuse_mapped.count > sanit_threshold) {
                list_node_t *n = list_pop_front(&p->vm.reuse_mapped);
                b_sanit = list_entry(n, struct vm_block, node);
                break;
            }
        }
    } else {
        b_sanit = b;
    }

    if (b_sanit) {
        //kprintf("To sanitize block @ %lx\n", b_sanit->base);
        list_insert_sorted_exclusive(&p->vm.sanit_mapped, &b_sanit->node, list_compare);
        ulong wait_acks = request_tlb_shootdown(p, b_sanit);
        if (!wait_acks) {
            // TODO: merge
        }
    }

    return 0;
}

int vm_free_block(struct process *p, struct vm_block *b)
{
    list_remove_exclusive(&p->vm.inuse_mapped, &b->node);
    return _vm_free_removed_block(p, b);
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

    panic_if(!b_free, "Unable to free VM block @ %lx\n", base);
    return b_free ? vm_free_block(p, b_free) : -1;
}


/*
 * Free pages
 */
static void vm_free_block_pages(struct process *p, struct vm_block *b)
{
    paddr_t prev_paddr = 0;
    for (ulong offset = 0; offset < b->size; offset += PAGE_SIZE) {
        ulong vaddr = b->base + offset;
        paddr_t paddr = 0;

        spinlock_exclusive_int(&p->page_table_lock) {
            paddr = get_hal_exports()->translate(p->page_table, vaddr);
            if (paddr && paddr != prev_paddr) {
                get_hal_exports()->unmap_range(p->page_table, vaddr, paddr, PAGE_SIZE);
            }
        }

        if (paddr && paddr != prev_paddr) {
            if (b->map_type == VM_MAP_TYPE_OWNER) {
                //kprintf("to free @ %x -> %llx\n", vaddr, (u64)paddr);
                pfree_paddr(paddr);
                ref_count_dec(&p->vm.num_palloc_pages);
            }

            prev_paddr = paddr;
        }
    }
}



/*
 * Move to avail list
 */
void vm_move_sanit_to_avail(struct process *p)
{
    list_node_t *n = list_pop_front_exclusive(&p->vm.sanit_unmapped);
    while (n) {
        struct vm_block *b = list_entry(n, struct vm_block, node);
        vm_free_block_pages(p, b);

        list_insert_merge_free_sorted_exclusive(
            &p->vm.avail_unmapped, n,
            list_compare, avail_list_merge, avail_list_free
        );

        ref_count_dec(&p->vm.num_active_blocks);

        n = list_pop_front_exclusive(&p->vm.sanit_unmapped);
    }
}


/*
 * Free up all inuse blocks
 */
ulong vm_purge(struct process *p)
{
    ulong count = 0;

    list_node_t *n = list_pop_front_exclusive(&p->vm.inuse_mapped);
    while (n) {
        struct vm_block *b = list_entry(n, struct vm_block, node);
        _vm_free_removed_block(p, b);
        count++;

        n = list_pop_front_exclusive(&p->vm.inuse_mapped);
    }

    return count;
}

void vm_destory(struct process *p)
{
    if (p->vm.avail_unmapped.count > 1) {
        kprintf("VM block may not have been purged!\n");
    }

    if (!ref_count_is_zero(&p->vm.num_palloc_pages)) {
        kprintf("Remaining unfreed pages: %lu\n", p->vm.num_palloc_pages.value);
    }

    list_node_t *n = list_pop_front_exclusive(&p->vm.avail_unmapped);
    while (n) {
        struct vm_block *b = list_entry(n, struct vm_block, node);
        sfree_audit(b, &p->vm.num_salloc_objs);

        n = list_pop_front_exclusive(&p->vm.avail_unmapped);
    }

    if (!ref_count_is_zero(&p->vm.num_salloc_objs)) {
        kprintf("Remaining unfreed salloc objs: %lu\n", p->vm.num_salloc_objs.value);
    }
}


/*
 * TLB shootdown done
 */
void vm_move_to_sanit_unmapped(struct process *p, struct vm_block *b)
{
    list_remove_exclusive(&p->vm.sanit_mapped, &b->node);
    list_push_back_exclusive(&p->vm.sanit_unmapped, &b->node);
    //list_insert_sorted_exclusive(&p->vm.sanit_unmapped, &b->node, list_compare);
}


/*
 * Map
 */
// Page fault handler
int vm_map(struct process *p, ulong addr, ulong prot)
{
    int err = -1;

    // FIXME: move palloc out of critical section

    list_foreach_exclusive(&p->vm.inuse_mapped, n) {
        struct vm_block *b = list_entry(n, struct vm_block, node);
        ulong end = b->base + b->size;
        if (addr >= b->base && addr < end) {
            ulong page_base = align_down_ulong(addr, PAGE_SIZE);

            paddr_t paddr = palloc_paddr(1);
            ref_count_inc(&p->vm.num_palloc_pages);

            spinlock_exclusive_int(&p->page_table_lock) {
                get_hal_exports()->map_range(p->page_table,
                                            page_base,
                                            paddr,
                                            PAGE_SIZE,
                                            1, 1, 1, 0, 0);
            }

            switch (b->map_type) {
            case VM_MAP_TYPE_NONE:
                b->map_type = VM_MAP_TYPE_OWNER;
            case VM_MAP_TYPE_OWNER:
                break;
            default:
                panic("Unexpected page fault!\n");
                break;
            }

            err = 0;
            break;
        }
    }

    return err;
}

// Map coreimg into process
ulong vm_map_coreimg(struct process *p)
{
    void *ci = get_coreimg();
    if (!ci) {
        return 0;
    }

    ulong size = coreimg_size();
    if (!size) {
        return 0;
    }

    ulong kernel_vaddr_start = align_down_vaddr((ulong)ci, PAGE_SIZE);
    ulong kernel_vaddr_end = align_up_vaddr((ulong)ci + size, PAGE_SIZE);
    ulong map_size = kernel_vaddr_end - kernel_vaddr_start;

    struct vm_block *b = vm_alloc(p, 0, map_size, 0);
    if (!b) {
        return 0;
    }

    paddr_t paddr = hal_cast_kernel_ptr_to_paddr((void *)kernel_vaddr_start);
    b->map_type = VM_MAP_TYPE_GUEST;
    spinlock_exclusive_int(&p->page_table_lock) {
        get_hal_exports()->map_range(p->page_table, b->base,
                                     paddr, b->size,
                                     1, 1, 1, 0, 0);
    }

    ulong offset = (ulong)ci - kernel_vaddr_start;
    return b->base + offset;
}

// Map devtree into process
ulong vm_map_devtree(struct process *p)
{
    void *dt = devtree_get_head();
    if (!dt) {
        return 0;
    }

    ulong size = devtree_get_buf_size();
    if (!size) {
        return 0;
    }

    ulong kernel_vaddr_start = align_down_vaddr((ulong)dt, PAGE_SIZE);
    ulong kernel_vaddr_end = align_up_vaddr((ulong)dt + size, PAGE_SIZE);
    ulong map_size = kernel_vaddr_end - kernel_vaddr_start;

    kprintf("devtree paddr @ %lx - %lx\n", kernel_vaddr_start, kernel_vaddr_end);

    struct vm_block *b = vm_alloc(p, 0, map_size, 0);
    if (!b) {
        return 0;
    }

    paddr_t paddr = hal_cast_kernel_ptr_to_paddr((void *)kernel_vaddr_start);
    b->map_type = VM_MAP_TYPE_GUEST;
    spinlock_exclusive_int(&p->page_table_lock) {
        get_hal_exports()->map_range(p->page_table, b->base,
                                     paddr, b->size,
                                     1, 1, 1, 0, 0);
    }

    ulong offset = (ulong)dt - kernel_vaddr_start;
    return b->base + offset;
}

// Map device MMIO into process
ulong vm_map_dev(struct process *p, ulong ppfn, ulong size, ulong prot)
{
    paddr_t paddr_start = ppfn_to_paddr((ppfn_t)ppfn);
    ulong map_size = align_up_vaddr(size, PAGE_SIZE);

    struct vm_block *b = vm_alloc(p, 0, map_size, 0);
    if (!b) {
        return 0;
    }

    b->map_type = VM_MAP_TYPE_GUEST;
    spinlock_exclusive_int(&p->page_table_lock) {
        get_hal_exports()->map_range(p->page_table, b->base,
                                     paddr_start, b->size,
                                     0, 1, 1, 0, 0); // Uncacheable
    }

    return b->base;
}

// Map kernel-accessible physical memory into process
ulong vm_map_kernel(struct process *p, ulong size, ulong prot)
{
    ulong map_size = align_up_vsize(size, PAGE_SIZE);
    struct vm_block *b = vm_alloc(p, 0, map_size, 0);
    if (!b) {
        return 0;
    }

    b->map_type = VM_MAP_TYPE_OWNER;
    for (ulong offset = 0; offset < map_size; offset += PAGE_SIZE) {
        paddr_t paddr = palloc_paddr_direct_mapped(1);
        panic_if(!paddr, "Unable to allocate page!\n");

        ref_count_inc(&p->vm.num_palloc_pages);
        spinlock_exclusive_int(&p->page_table_lock) {
            get_hal_exports()->map_range(p->page_table, b->base + offset,
                                         paddr, PAGE_SIZE,
                                         1, 1, 1, 0, 0);
        }
    }

    return b->base;
}

// Map shared memory block in two processes
ulong vm_map_cross(struct process *p, ulong remote_pid,
                   ulong remote_vbase, ulong size, ulong remote_prot)
{
    ulong local_vbase = 0;
    ulong align_offset = 0;

    access_process(remote_pid, remote_p) {
        ulong remote_map_vbase = align_down_vaddr(remote_vbase, PAGE_SIZE);
        ulong remote_map_vend = align_up_vaddr(remote_vbase + size, PAGE_SIZE);
        ulong map_size = remote_map_vend - remote_map_vbase;

        //kprintf("remote vbase @ %lx, remote vend @ %lx\n", remote_map_vbase, remote_map_vend);

        struct vm_block *remote_b = vm_alloc(remote_p, remote_map_vbase, map_size, remote_prot);
        if (!remote_b) break;
        remote_b->map_type = VM_MAP_TYPE_OWNER;
        align_offset = remote_vbase - remote_map_vbase;

        struct vm_block *local_b = vm_alloc(p, 0, map_size, 0);
        panic_if(!local_b, "Unable to allocate VM block!\n");
        local_b->map_type = VM_MAP_TYPE_GUEST;
        local_vbase = local_b->base;

        //kprintf("b @ %p, base: %lx, size: %lx\n", local_b, local_b->base, local_b->size);

        for (ulong offset = 0; offset < map_size; offset += PAGE_SIZE) {
            paddr_t paddr = palloc_paddr(1);
            ref_count_inc(&remote_p->vm.num_palloc_pages);

            ulong remote_vaddr = remote_map_vbase + offset;
            spinlock_exclusive_int(&remote_p->page_table_lock) {
                get_hal_exports()->map_range(remote_p->page_table, remote_vaddr,
                                             paddr, PAGE_SIZE,
                                             1, 1, 1, 0, 0);
            }

            ulong local_vaddr = local_vbase + offset;
            spinlock_exclusive_int(&p->page_table_lock) {
                get_hal_exports()->map_range(p->page_table, local_vaddr,
                                             paddr, PAGE_SIZE,
                                             1, 1, 1, 0, 0);
            }
        }
    }

    //kprintf("local vbase @ %lx, align offset: %lx\n", local_vbase, align_offset);

    return local_vbase ? local_vbase + align_offset : 0;

}
