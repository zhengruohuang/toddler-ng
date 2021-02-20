// #include "common/include/inttypes.h"
// #include "common/include/page.h"
// #include "hal/include/lib.h"
// #include "hal/include/kernel.h"
// #include "hal/include/hal.h"
// #include "hal/include/mem.h"
// #include "hal/include/setup.h"
//
//
// /*
//  * Page table
//  */
// void *init_user_page_table()
// {
//     void *page_table = kernel_palloc_ptr(1);
//     memzero(page_table, PAGE_SIZE);
//     return page_table;
// }
//
// void free_user_page_table(void *ptr)
// {
//     kernel_pfree_ptr(ptr);
// }
//
//
// /*
//  * Get physical address of a user virtual address
//  */
// paddr_t translate_attri(void *page_table, ulong vaddr,
//                         int *exec, int *read, int *write, int *cache)
// {
//     // L1 table
//     struct page_frame *l1tab = page_table;
//     int index = GET_L1PTE_INDEX(vaddr);
//     struct page_table_entry *l1entry = &l1tab->entries[index];
//
//     // Not mapped yet
//     if (!l1entry->value) {
//         return 0;
//     }
//
//     // 1MB section
//     if (l1entry->present && l1entry->block) {
//         ppfn_t bfn = l1entry->bfn;
//         paddr_t paddr = pbfn_to_paddr(bfn);
//         paddr |= (paddr_t)get_vblock_offset(vaddr);
//
//         if (exec) *exec = l1entry->exec_allow;
//         if (read) *read = l1entry->read_allow;
//         if (write) *write = l1entry->write_allow;
//         if (cache) *cache = l1entry->cache_allow;
//
//         return paddr;
//     }
//
//     // L2 table
//     assert(l1entry->present);
//     ppfn_t pfn = l1entry->pfn;
//     paddr_t paddr = ppfn_to_paddr(pfn);
//     struct page_frame *l2tab = cast_paddr_to_cached_seg(paddr);
//
//     index = GET_L2PTE_INDEX(vaddr);
//     struct page_table_entry *l2entry = &l2tab->entries[index];
//
//     // Not mapped yet
//     if (!l2entry->value) {
//         return 0;
//     }
//
//     // Paddr
//     pfn = l2entry->pfn;
//     paddr = ppfn_to_paddr(pfn);
//     paddr |= (paddr_t)get_vpage_offset(vaddr);
//
//     if (exec) *exec = l2entry->exec_allow;
//     if (read) *read = l2entry->read_allow;
//     if (write) *write = l2entry->write_allow;
//     if (cache) *cache = l2entry->cache_allow;
//
//     return paddr;
// }
//
// paddr_t translate(void *page_table, ulong vaddr)
// {
//     return translate_attri(page_table, vaddr, NULL, NULL, NULL, NULL);
// }
//
//
// /*
//  * Map
//  */
// static struct page_frame *alloc_page_frame(palloc_t palloc, ppfn_t *pfn)
// {
//     ppfn_t ppfn = palloc(1);
//     if (pfn) {
//         *pfn = ppfn;
//     }
//
//     paddr_t paddr = ppfn_to_paddr(ppfn);
//     void *ptr = cast_paddr_to_cached_seg(paddr);
//     memzero(ptr, PAGE_SIZE);
//
//     return ptr;
// }
//
// static void map_page(void *page_table, ulong vaddr, paddr_t paddr, int block,
//                      int cache, int exec, int write, int kernel, int override,
//                      palloc_t palloc)
// {
//     struct page_frame *page = page_table;
//     struct page_table_entry *entry = NULL;
//
//     for (int level = 0; level <= block; level++) {
//         int idx = (int)GET_PTE_INDEX((ulong)vaddr, level);
//         entry = &page->entries[idx];
//
//         if (level != block) {
//             if (!entry->valid) {
//                 page = alloc_page();
//                 memzero(page, sizeof(struct page_frame));
//
//                 entry->valid = 1;
//                 entry->pfn = ADDR_TO_PFN((ulong)page);
//             } else {
//                 page = (void *)PFN_TO_ADDR((ulong)entry->pfn);
//             }
//         }
//     }
//
//     if (entry->valid) {
//         panic_if((ulong)entry->pfn != (ulong)ADDR_TO_PFN(paddr) ||
//             entry->read != !1 ||
//             entry->write != write ||
//             entry->exec != exec,
//             "Trying to change an existing mapping @ %p"
//             ", existing PFN @ %lx, new PFN @ %lx!",
//             vaddr, (ulong)entry->pfn, (ulong)ADDR_TO_PFN(paddr));
//     } else {
//         entry->valid = 1;
//         entry->pfn = (ulong)ADDR_TO_PFN(paddr);
//
//         entry->read = 1;
//         entry->write = write;
//         entry->exec = exec;
//     }
// }
//
// int map_range(void *page_table, void *vaddr, void *ppaddr, ulong size,
//               int cache, int exec, int write,, int kernel, int override,
//               palloc_t palloc)
// {
//     kprintf("Map range @ %p -> %p, size: %lx, cache: %d, exec: %d, write: %d\n",
//         vaddr, ppaddr, size, cache, exec, write);
//
//     u64 paddr = (ulong)ppaddr;
//     if (cache) {
//         verify_cacheability(paddr, size);
//     }
//
//     ulong vaddr_start = ALIGN_DOWN((ulong)vaddr, PAGE_SIZE);
//     ulong vaddr_end = ALIGN_UP((ulong)vaddr + size, PAGE_SIZE);
//     u64 paddr_start = ALIGN_DOWN(paddr, PAGE_SIZE);
//
//     int mapped_pages = 0;
//
//     u64 cur_paddr = paddr_start;
//     for (ulong cur_vaddr = vaddr_start; cur_vaddr < vaddr_end; ) {
//         // L0 block
//         if (ALIGNED(cur_vaddr, L0BLOCK_SIZE) &&
//             ALIGNED(cur_paddr, L0BLOCK_SIZE) &&
//             vaddr_end - cur_vaddr >= L0BLOCK_SIZE
//         ) {
//             map_page(page_table, (void *)cur_vaddr, cur_paddr, 0,
//                 cache, exec, write);
//
//             mapped_pages += L0BLOCK_PAGE_COUNT;
//             cur_vaddr += L0BLOCK_SIZE;
//             cur_paddr += L0BLOCK_SIZE;
//         }
//
// #if (defined(ARCH_RISCV64))
//         // L1 block
//         else if (ALIGNED(cur_vaddr, L1BLOCK_SIZE) &&
//             ALIGNED(cur_paddr, L1BLOCK_SIZE) &&
//             vaddr_end - cur_vaddr >= L1BLOCK_SIZE
//         ) {
//             map_page(page_table, (void *)cur_vaddr, cur_paddr, 1,
//                 cache, exec, write);
//
//             mapped_pages += L1BLOCK_PAGE_COUNT;
//             cur_vaddr += L1BLOCK_SIZE;
//             cur_paddr += L1BLOCK_SIZE;
//         }
//
// #   if (VADDR_BITS == 48)
//         // L2 block
//         else if (ALIGNED(cur_vaddr, L2BLOCK_SIZE) &&
//             ALIGNED(cur_paddr, L2BLOCK_SIZE) &&
//             vaddr_end - cur_vaddr >= L2BLOCK_SIZE
//         ) {
//             map_page(page_table, (void *)cur_vaddr, cur_paddr, 2,
//                 cache, exec, write);
//
//             mapped_pages += L2BLOCK_PAGE_COUNT;
//             cur_vaddr += L2BLOCK_SIZE;
//             cur_paddr += L2BLOCK_SIZE;
//         }
// #   endif
// #endif
//
//         // 4KB page
//         else {
//             map_page(page_table, (void *)cur_vaddr, cur_paddr,
//                 PAGE_LEVELS - 1, cache, exec, write);
//
//             mapped_pages++;
//             cur_vaddr += PAGE_SIZE;
//             cur_paddr += PAGE_SIZE;
//         }
//     }
//
//     return mapped_pages;
// }
//
//
// /*
//  * Unmap
//  */
// static void unmap_page(void *page_table, ulong vaddr, paddr_t paddr, int block)
// {
//     //kprintf("unmap page, vaddr @ %lx, paddr @ %llx\n", vaddr, (u64)paddr);
//
//     // L1 table
//     struct page_frame *l1tab = page_table;
//     int l1index = GET_L1PTE_INDEX(vaddr);
//     struct page_table_entry *l1entry = &l1tab->entries[l1index];
//     assert(l1entry->value);
//
//     if (block) {
//         assert(l1entry->present);
//         assert(l1entry->block);
//
//         // Unmap L1 entry
//         assert(l1entry->bfn == (u32)paddr_to_pbfn(paddr));
//         l1entry->value = 0;
//     } else {
//         assert(l1entry->present);
//         assert(!l1entry->block);
//
//         ppfn_t l2_pfn = l1entry->pfn;
//         paddr_t l2_paddr = ppfn_to_paddr(l2_pfn);
//         struct page_frame *l2tab = cast_paddr_to_cached_seg(l2_paddr);
//
//         int l2index = GET_L2PTE_INDEX(vaddr);
//         struct page_table_entry *l2entry = &l2tab->entries[l2index];
//         assert(l2entry->present);
//
//         // Unmap L2 entry
//         assert(l2entry->pfn == (u32)paddr_to_ppfn(paddr));
//         l2entry->value = 0;
//
//         // Free L2 page if needed
//         int free_l2 = 1;
//         for (int i = 0; i < PAGE_TABLE_ENTRY_COUNT; i++) {
//             if (l2tab->entries[i].value) {
//                 free_l2 = 0;
//                 break;
//             }
//         }
//
//         if (free_l2) {
//             int num_free_pages = kernel_pfree(l2_pfn);
//             panic_if(num_free_pages != 1, "Inconsistent num of freed pages");
//
//             l1entry->value = 0;
//         }
//     }
// }
//
// int unmap_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size)
// {
//     //kprintf("To unmap, pfn: %u, vaddr: %u, paddr: %u, size: %u\n",
//     //        page_dir_pfn, vaddr, paddr, length);
//
//     ulong vaddr_start = align_down_vaddr(vaddr, PAGE_SIZE);
//     paddr_t paddr_start = align_down_paddr(paddr, PAGE_SIZE);
//     ulong vaddr_end = align_up_vaddr(vaddr + size, PAGE_SIZE);
//
//     int unmapped_pages = 0;
//
//     paddr_t cur_paddr = paddr_start;
//     for (ulong cur_vaddr = vaddr_start; cur_vaddr < vaddr_end;) {
//         // 1MB block
//         if (is_vaddr_aligned(cur_vaddr, L1BLOCK_SIZE) &&
//             is_paddr_aligned(cur_paddr, L1BLOCK_SIZE) &&
//             vaddr_end - cur_vaddr >= L1BLOCK_SIZE
//         ) {
//             unmap_page(page_table, cur_vaddr, cur_paddr, 1);
//
//             unmapped_pages += L1BLOCK_PAGE_COUNT;
//             cur_vaddr += L1BLOCK_SIZE;
//             cur_paddr += L1BLOCK_SIZE;
//         }
//
//         // 4KB page
//         else {
//             unmap_page(page_table, cur_vaddr, cur_paddr, 0);
//
//             unmapped_pages++;
//             cur_vaddr += PAGE_SIZE;
//             cur_paddr += PAGE_SIZE;
//         }
//     }
//
//     return unmapped_pages;
// }
