/*
 * TLB shootdown
 */


#include "common/include/inttypes.h"
#include "common/include/errno.h"
#include "kernel/include/mem.h"
#include "kernel/include/struct.h"
#include "kernel/include/kernel.h"
#include "kernel/include/proc.h"


/*
 * Queue and sequence number
 */
static list_t tlb_shootdown_reqs;

static volatile ulong tlb_shootdown_seq = 1;
static decl_per_cpu(ulong, local_tlb_shootdown_seq);

static int tlb_shootdown_list_compare(list_node_t *a, list_node_t *b)
{
    struct vm_block *ba = list_entry(a, struct vm_block, node_tlb_shootdown);
    struct vm_block *bb = list_entry(b, struct vm_block, node_tlb_shootdown);
    if (ba->tlb_shootdown_seq > bb->tlb_shootdown_seq) {
        return 1;
    } else if (ba->tlb_shootdown_seq < bb->tlb_shootdown_seq) {
        return -1;
    } else {
        return 0;
    }
}

static inline ulong get_my_cpu_seq()
{
    ulong *my_seq_ptr = get_per_cpu(ulong, local_tlb_shootdown_seq);
    return *my_seq_ptr;
}

static inline ulong set_my_cpu_seq(ulong seq)
{
    ulong *my_seq_ptr = get_per_cpu(ulong, local_tlb_shootdown_seq);
    *my_seq_ptr = seq;
    return seq;
}

static inline ulong alloc_tlb_shootdown_seq()
{
    ulong seq = atomic_fetch_and_add(&tlb_shootdown_seq, 1);
    return seq;
}


/*
 * TLB invalidate
 */
static inline void shootdown(ulong asid, ulong base, ulong size)
{
    get_hal_exports()->invalidate_tlb(asid, base, size);
}


/*
 * Init
 */
void init_tlb_shootdown()
{
    // Only initialize this on the bootstrap CPU,
    // since all we need is allocating some space in the per-cpu buffer
    ulong *my_seq_ptr = get_per_cpu(ulong, local_tlb_shootdown_seq);
    *my_seq_ptr = 0;

    list_init(&tlb_shootdown_reqs);
}


/*
 * Request
 */
int request_tlb_shootdown(struct process *p, struct vm_block *b)
{
    int num_cpus = hal_get_num_cpus();
    if (num_cpus <= 1) {
        shootdown(p->asid, b->base, b->size);
        vm_move_to_sanit_unmapped(p, b);
        return 0;
    }

    b->tlb_shootdown_seq = alloc_tlb_shootdown_seq();
    b->wait_acks = num_cpus;

    list_insert_sorted_exclusive(&tlb_shootdown_reqs, &b->node_tlb_shootdown, tlb_shootdown_list_compare);
    return num_cpus;
}


/*
 * Service
 */
//static volatile long debug_print_count = 1000;

void service_tlb_shootdown_requests()
{
    atomic_mb();
    ulong my_seq = get_my_cpu_seq();

    list_foreach_exclusive(&tlb_shootdown_reqs, n) {
        struct vm_block *b = list_entry(n, struct vm_block, node_tlb_shootdown);
        if (my_seq < b->tlb_shootdown_seq) {
            shootdown(b->proc->asid, b->base, b->size);
            atomic_dec(&b->wait_acks);
            atomic_mb();

            my_seq = set_my_cpu_seq(b->tlb_shootdown_seq);
        }
    }

    list_access_exclusive(&tlb_shootdown_reqs) {
        while (tlb_shootdown_reqs.count) {
            list_node_t *n = list_front(&tlb_shootdown_reqs);
            struct vm_block *b = list_entry(n, struct vm_block, node_tlb_shootdown);

            atomic_mb();
            if (b->wait_acks) {
                break;
            }

            //kprintf("VM block unmapped @ %lx, seq: %lu, awaiting: %d\n",
            //        b->base, b->tlb_shootdown_seq, tlb_shootdown_reqs.count);

            list_pop_front(&tlb_shootdown_reqs);
            vm_move_to_sanit_unmapped(b->proc, b);
        }
    }
}
