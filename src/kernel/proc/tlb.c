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
 * TLB shootdown requests
 */
static list_t tlbsd_reqs;
static volatile ulong tlbsd_seq = 1;

struct tlbsd_req {
    list_node_t node;

    ulong asid;
    ulong base;
    ulong size;

    struct process *p;
    struct dynamic_block *b;

    volatile ulong seq;
    volatile ulong remain_acks;
};

static inline ulong get_my_cpu_seq()
{
    return 0;
}

static inline void set_my_cpu_seq(ulong seq)
{
}

static inline ulong alloc_tlbsd_seq()
{
    ulong seq = atomic_fetch_and_add(&tlbsd_seq, 1);
    return seq + 1;
}

void init_tlb_shootdown()
{
    list_init(&tlbsd_reqs);
}

static inline void shootdown(ulong asid, ulong base, ulong size)
{
}

void service_tlb_shootdown_requests()
{
    ulong my_seq = get_my_cpu_seq();
    list_foreach_exclusive(&tlbsd_reqs, n) {
        struct vm_block *b = list_entry(n, struct vm_block, node);
        if (my_seq <= b->tlb_shootdown_seq) {
            shootdown(b->proc->asid, b->base, b->size);
            set_my_cpu_seq(b->tlb_shootdown_seq);
            b->wait_acks--;
        }
    }

    list_access_exclusive(&tlbsd_reqs) {
        while (tlbsd_reqs.count) {
            list_node_t *n = list_front(&tlbsd_reqs);
            struct vm_block *b = list_entry(n, struct vm_block, node);
            if (b->wait_acks) {
                break;
            }

            list_pop_front(&tlbsd_reqs);
            vm_move_to_sanit_unmapped(b->proc, b);
        }
    }
}

int request_tlb_shootdown(struct process *p, struct vm_block *b)
{
    int num_cpus = hal_get_num_cpus();
    if (num_cpus <= 1) {
        shootdown(p->asid, b->base, b->size);
        vm_move_to_sanit_unmapped(p, b);
        return 0;
    }

    b->tlb_shootdown_seq = alloc_tlbsd_seq();
    b->wait_acks = num_cpus;

    list_push_back_exclusive(&tlbsd_reqs, &b->node_tlb_shootdown);
    return num_cpus;
}
