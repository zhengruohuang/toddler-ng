#ifndef __LIBK_INCLUDE_MEM_H__
#define __LIBK_INCLUDE_MEM_H__


#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "libk/include/debug.h"
#include "libk/include/bit.h"


/*
 * Align up and down
 */
static inline paddr_t align_up_paddr(paddr_t paddr, int align)
{
    return ALIGN_UP(paddr, (paddr_t)align);
}

static inline paddr_t align_down_paddr(paddr_t paddr, int align)
{
    return ALIGN_DOWN(paddr, (paddr_t)align);
}

static inline ulong align_up_vaddr(ulong vaddr, int align)
{
    return ALIGN_UP(vaddr, (ulong)align);
}

static inline ulong align_down_vaddr(ulong vaddr, int align)
{
    return ALIGN_DOWN(vaddr, (ulong)align);
}

static inline void *align_up_ptr(void *ptr, int align)
{
    return (void *)ALIGN_UP((ulong)ptr, (ulong)align);
}

static inline void *align_down_ptr(void *ptr, int align)
{
    return (void *)ALIGN_DOWN((ulong)ptr, (ulong)align);
}

static inline psize_t align_up_psize(psize_t psize, int align)
{
    return ALIGN_UP(psize, (psize_t)align);
}

static inline psize_t align_down_psize(psize_t psize, int align)
{
    return ALIGN_DOWN(psize, (psize_t)align);
}

static inline ulong align_up_vsize(ulong vsize, int align)
{
    return ALIGN_UP(vsize, (psize_t)align);
}

static inline ulong align_down_vsize(ulong vsize, int align)
{
    return ALIGN_DOWN(vsize, (psize_t)align);
}


/*
 * Is aligned
 */
static inline int is_paddr_aligned(paddr_t paddr, int align)
{
    return ALIGNED(paddr, (paddr_t)align);
}

static inline int is_vaddr_aligned(ulong vaddr, int align)
{
    return ALIGNED(vaddr, (ulong)align);
}

static inline int is_ptr_aligned(void *ptr, int align)
{
    return ALIGNED((ulong)ptr, (ulong)align);
}


/*
 * Cast
 */
static inline paddr_t cast_vaddr_to_paddr(ulong vaddr)
{
    int bits = 64 - clz64((u64)vaddr);
    panic_if(bits > MAX_NUM_PADDR_BITS, "Unable to cast vaddr to paddr!\n");
    return (paddr_t)vaddr;
}

static inline paddr_t cast_ptr_to_paddr(void *ptr)
{
    return cast_vaddr_to_paddr((ulong)ptr);
}

static inline paddr_t cast_u64_to_paddr(u64 addr)
{
    int bits = 64 - clz64(addr);
    panic_if(bits > MAX_NUM_PADDR_BITS, "Unable to cast vaddr to paddr!\n");
    return (paddr_t)addr;
}

static inline paddr_t cast_u32_to_paddr(u32 addr)
{
    int bits = 32 - clz32(addr);
    panic_if(bits > MAX_NUM_PADDR_BITS, "Unable to cast vaddr to paddr!\n");
    return (paddr_t)addr;
}

static inline ulong cast_paddr_to_vaddr(paddr_t paddr)
{
    int bits = 64 - clz64((u64)paddr);
    panic_if(bits > MAX_NUM_VADDR_BITS, "Unable to cast paddr to vaddr!\n");
    return (ulong)paddr;
}

static inline void *cast_paddr_to_ptr(paddr_t paddr)
{
    return (void *)cast_paddr_to_vaddr(paddr);
}

static inline u64 cast_paddr_to_u64(paddr_t paddr)
{
    return (u64)paddr;
}

static inline u32 cast_paddr_to_u32(paddr_t paddr)
{
    int bits = 64 - clz64((u64)paddr);
    panic_if(bits > 32, "Unable to cast paddr to u32!\n");
    return (u32)paddr;
}


/*
 * ADDR <--> PFN
 */
static inline ppfn_t paddr_to_ppfn(paddr_t paddr)
{
    return paddr >> PAGE_BITS;
}

static inline ppfn_t ppfn_to_paddr(paddr_t ppfn)
{
    return ppfn << PAGE_BITS;
}

static inline ulong vaddr_to_vpfn(ulong vaddr)
{
    return vaddr >> PAGE_BITS;
}

static inline ulong vpfn_to_vaddr(ulong vpfn)
{
    return vpfn << PAGE_BITS;
}

static inline ulong ptr_to_vpfn(void *ptr)
{
    return ((ulong)ptr) >> PAGE_BITS;
}

static inline void *vpfn_to_ptr(ulong vpfn)
{
    return (void *)(vpfn << PAGE_BITS);
}


/*
 * Page count
 */
static inline psize_t get_ppage_count(psize_t psize)
{
    return align_up_psize(psize, PAGE_SIZE) >> PAGE_BITS;
}

static inline ulong get_vpage_count(ulong vsize)
{
    return align_up_vsize(vsize, PAGE_SIZE) >> PAGE_BITS;
}

static inline ulong cast_ppage_count_to_vpage_count(psize_t ppage_count)
{
    int bits = 64 - clz64((u64)ppage_count);
    panic_if(bits >= sizeof(ulong) * 8, "Unable to cast paddr to ulong!\n");
    return (ulong)ppage_count;
}


/*
 * Page offset
 */
static inline ulong get_ppage_offset(paddr_t paddr)
{
    return (ulong)(paddr & ((paddr_t)PAGE_SIZE - 1));
}

static inline ulong get_vpage_offset(ulong vaddr)
{
    return vaddr & ((ulong)PAGE_SIZE - 1);
}


#endif
