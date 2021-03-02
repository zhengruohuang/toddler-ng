#ifndef __LIBK_INCLUDE_PAGE_H__
#define __LIBK_INCLUDE_PAGE_H__


#include "common/include/mem.h"

typedef ppfn_t (*page_table_palloc_t)(int count);
typedef int (*page_table_pfree_t)(ppfn_t ppfn);
typedef void *(*page_table_page_paddr_to_ptr)(paddr_t paddr);


#if (defined(USE_GENERIC_PAGE_TABLE) && USE_GENERIC_PAGE_TABLE)

extern void init_libk_page_table(page_table_palloc_t pa, page_table_pfree_t pf, page_table_page_paddr_to_ptr pp);
extern void init_libk_page_table_alloc(page_table_palloc_t pa, page_table_pfree_t pf);

extern paddr_t generic_translate_attri(void *page_table, ulong vaddr, int *exec, int *read, int *write, int *cache, int *kernel);
extern paddr_t generic_translate(void *page_table, ulong vaddr);

extern int generic_map_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size,
                             int cache, int exec, int write, int kernel, int override);
extern int generic_unmap_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size);

#endif


#endif
