#ifndef __KERNEL_INCLUDE_MEM__
#define __KERNEL_INCLUDE_MEM__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


/*
 * PFN database
 */
struct pfndb_entry {
    union {
        u16 flags;

        struct {
            u16 usable      : 1;
            u16 mapped      : 1;
            u16 tag         : 4;
            u16 inuse       : 1;
            u16 zeroed      : 1;
            u16 kernel      : 1;
            u16 swappable   : 1;
        };
    };
} packed2_struct;

extern struct pfndb_entry *get_pfn_entry_by_pfn(ulong pfn);
extern struct pfndb_entry *get_pfn_entry_by_paddr(ulong paddr);

extern ulong get_mem_range(ulong *start);
extern ulong get_pfn_range(ulong *start);

extern ulong reserve_free_pages(ulong count);
extern ulong reserve_free_mem(ulong size);

extern void init_pfndb();


/*
 * Page frame allocator
 */
extern ulong palloc_tag(int count, int tag);
extern ulong palloc(int count);
extern int pfree(ulong pfn);

extern void init_palloc();

extern void buddy_print();
extern void test_palloc();


// /*
//  * Struct allocator
//  */
// typedef void (*salloc_callback_t)(void* entry);
//
// extern void init_salloc();
// extern int salloc_create(size_t size, size_t align, int count, salloc_callback_t construct, salloc_callback_t destruct);
// extern void *salloc(int obj_id);
// extern void sfree(void *ptr);
//
//
// /*
//  * General memory allocator
//  */
// extern void init_malloc();
// extern void *malloc(size_t size);
// extern void *calloc(int count, size_t size);
// extern void free(void *ptr);
// extern void test_malloc();


#endif
