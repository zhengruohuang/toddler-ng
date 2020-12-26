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

static inline ulong get_my_cpu_seq()
{
    ulong *my_seq_ptr = get_per_cpu(ulong, local_tlb_shootdown_seq);
    return *my_seq_ptr;
}

static inline void set_my_cpu_seq(ulong seq)
{
    ulong *my_seq_ptr = get_per_cpu(ulong, local_tlb_shootdown_seq);
    *my_seq_ptr = seq;
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

    list_push_back_exclusive(&tlb_shootdown_reqs, &b->node_tlb_shootdown);
    return num_cpus;
}


/*
 * Service
 */
void service_tlb_shootdown_requests()
{
    ulong my_seq = get_my_cpu_seq();
    list_foreach_exclusive(&tlb_shootdown_reqs, n) {
        struct vm_block *b = list_entry(n, struct vm_block, node_tlb_shootdown);
        if (my_seq < b->tlb_shootdown_seq) {
            shootdown(b->proc->asid, b->base, b->size);
            set_my_cpu_seq(b->tlb_shootdown_seq);
            b->wait_acks--;
        }
    }

    list_access_exclusive(&tlb_shootdown_reqs) {
        while (tlb_shootdown_reqs.count) {  // FIXME: volatile?
            list_node_t *n = list_front(&tlb_shootdown_reqs);
            struct vm_block *b = list_entry(n, struct vm_block, node_tlb_shootdown);
            if (b->wait_acks) {
                break;
            }

            //kprintf("VM block unmapped @ %lx, seq: %lu, awaiting: %d\n", b->base, b->tlb_shootdown_seq, tlb_shootdown_reqs.count);

            list_pop_front(&tlb_shootdown_reqs);
            vm_move_to_sanit_unmapped(b->proc, b);
        }
    }
}
