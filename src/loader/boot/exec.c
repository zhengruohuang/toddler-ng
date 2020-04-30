#include "common/include/inttypes.h"
#include "common/include/abi.h"
#include "common/include/elf.h"
#include "loader/include/kprintf.h"
#include "loader/include/lib.h"
#include "loader/include/mem.h"
#include "loader/include/firmware.h"
#include "loader/include/boot.h"


/*
 * Native ELF
 */
#if ARCH_WIDTH == 32
typedef struct elf32_header     elf_native_header_t;
typedef struct elf32_program    elf_native_program_t;
typedef struct elf32_section    elf_native_section_t;
typedef Elf32_Addr              elf_native_addr_t;
#else
typedef struct elf64_header     elf_native_header_t;
typedef struct elf64_program    elf_native_program_t;
typedef struct elf64_section    elf_native_section_t;
typedef Elf64_Addr              elf_native_addr_t;
#endif


static inline void *rebase_to_win(void *addr,
    ulong target_win, ulong access_win)
{
    return addr + access_win - target_win;
}

static void get_vmem_range(elf_native_header_t *elf, ulong *start, ulong *end)
{
    int first = 1;
    elf_native_program_t *prog = NULL;

    for (int i = 0; i < elf->elf_phnum; i++) {
        if (i) {
            prog = (void *)prog + elf->elf_phentsize;
        } else {
            prog = (void *)elf + elf->elf_phoff;
        }

        if (prog->program_memsz) {
            if (start &&
                (first || prog->program_vaddr < *start)
            ) {
                *start = prog->program_vaddr;
            }

            if (end &&
                (first || prog->program_vaddr + prog->program_memsz > *end)
            ) {
                *end = prog->program_vaddr + prog->program_memsz;
            }

            first = 0;
        }
    }
}

static void inflate_elf(elf_native_header_t *elf,
    ulong target_win, ulong access_win)
{
    elf_native_program_t *prog = NULL;

    for (int i = 0; i < elf->elf_phnum; i++) {
        if (i) {
            prog = (void *)prog + elf->elf_phentsize;
        } else {
            prog = (void *)elf + elf->elf_phoff;
        }

        // Rebase target
        ulong target = (ulong)rebase_to_win((void *)(ulong)prog->program_vaddr,
                                            target_win, access_win);
        kprintf("target @ %lx, target_win @ %lx, access_win @ %lx\n", target, target_win, access_win);

        // Zero memory
        if (prog->program_memsz) {
            memzero((void *)target, prog->program_memsz);
        }

        // Copy program data
        if (prog->program_filesz) {
            kprintf("\tCopying: %p -> %p (win @ %p)\n",
                (void *)(ulong)(prog->program_offset + (ulong)elf),
                (void *)prog->program_vaddr, (void *)target);

            memcpy(
                (void *)target,
                (void *)(ulong)(prog->program_offset + (ulong)elf),
                prog->program_filesz
            );
        }
    }
}

static elf_native_section_t *find_section(elf_native_header_t *elf,
    const char *name)
{
    if (!elf->elf_shoff) {
        return NULL;
    }

    elf_native_section_t *sections = (void *)elf + elf->elf_shoff;
    if (elf->elf_shstrndx == ELF_SHN_UNDEF) {
        return NULL;
    }

    elf_native_section_t *name_sec = &sections[elf->elf_shstrndx];
    if (!strcmp(name, ".shstrtab")) {
       return name_sec;
    }

    const char *names = (void *)elf + name_sec->section_offset;
    for (int i = 0; i < elf->elf_shnum; i++) {
        elf_native_section_t *sec = &sections[i];
        if (!strcmp(&names[sec->section_name], name)) {
            return sec;
        }
    }

    return NULL;
}

// static void relocate_got(elf_native_header_t *elf,
//                          ulong target_win, ulong access_win)
// {
//     elf_native_section_t *sec = find_section(elf, ".got");
//     if (!sec) {
//         return;
//     }
//
//     panic_if(sec->section_entsize != sizeof(elf_native_addr_t),
//              "Unable to handle GOT entry size: %ld\n", (ulong)sec->section_entsize);
//
//     elf_native_addr_t *got = rebase_to_win((void *)(ulong)sec->section_addr,
//                                            target_win, access_win);
//
//     int num_entries = sec->section_size / sizeof(elf_native_addr_t);
//     for (int i = ELF_GOT_NUM_RESERVED_ENTRIES; i < num_entries; i++) {
//         //if (got[i]) {
//             kprintf("Relocate #%d @ %lx -> %p\n", i, (ulong)got[i],
//                     rebase_to_win((void *)(ulong)got[i], target_win, access_win));
//
//             got[i] = (ulong)rebase_to_win((void *)(ulong)got[i],
//                                         target_win, access_win);
//         //}
//     }
// }

static ulong load_elf(const char *name, ulong *range_start, ulong *range_end)
{
    elf_native_header_t *elf = coreimg_find_file(name);
    if (!elf) {
        return 0;
    }

    // Get range
    ulong vaddr_start = 0, vaddr_end = 0;
    get_vmem_range(elf, &vaddr_start, &vaddr_end);

    kprintf("ELF range @ %lx - %lx\n", vaddr_start, vaddr_end);

    // Align
    vaddr_start = align_down_vaddr(vaddr_start, PAGE_SIZE);
    vaddr_end = align_up_vaddr(vaddr_end, PAGE_SIZE);
    ulong vaddr_size = vaddr_end - vaddr_start;

    struct loader_arch_funcs *funcs = get_loader_arch_funcs();
    void *access_win = NULL;

    if (funcs->has_direct_access) {
        // Since direct access is present, no mapping is needed
        // The coresponding paddr can be directly obtained
        paddr_t paddr_start = funcs->access_win_to_phys((void *)vaddr_start);
        access_win = (void *)vaddr_start;

        // Pmem region has to be checked to make sure there is no overlap
        int usable = check_memmap_region_usable(cast_paddr_to_u64(paddr_start), (u64)vaddr_size);
        panic_if(!usable, "Paddr overlap!\n");

        // But the paddr range has to be claimeds
        claim_memmap_region(cast_paddr_to_u64(paddr_start), (u64)vaddr_size, MEMMAP_USED);
    } else {
        // Alloc phys and map phys mem, memmap region will be automatically claimed
        paddr_t paddr = memmap_alloc_phys(vaddr_size, PAGE_SIZE);
        kprintf("Paddr @ %llx\n", (u64)paddr);

        // Alloc and map virt mem accessible by loader as the access window
        access_win = firmware_alloc_and_map_acc_win(paddr, vaddr_size, PAGE_SIZE);
        kprintf("Win @ %p\n", access_win);

        // Set up mappings in page table
        int pages = page_map_virt_to_phys(vaddr_start, paddr, vaddr_size, 1, 1, 1);
        panic_if(!pages, "Unable to map vaddr!\n");

//         // Load ELF
//         inflate_elf(elf, vaddr_start, (ulong)access_win);
//
//         // If no pages were mapped, it means the on current arch HAL and kernel
//         // should be placed in unmapped memory region
//         // In this case, relocation is needed
//         if (!pages) {
//             relocate_got(elf, vaddr_start, (ulong)access_win);
//
//             if (range_start) *range_start = (ulong)access_win;
//             if (range_end) *range_end = (ulong)access_win + vaddr_size;
//
//             entry = elf->elf_entry + (ulong)access_win - vaddr_start;
//         } else {
//             if (range_start) *range_start = vaddr_start;
//             if (range_end) *range_end = vaddr_end;
//             entry = elf->elf_entry;
//         }
    }

    // Load ELF
    inflate_elf(elf, vaddr_start, (ulong)access_win);

    // Set up range and entry
    if (range_start) *range_start = vaddr_start;
    if (range_end) *range_end = vaddr_end;
    ulong entry = elf->elf_entry;

    kprintf("Entry @ %lx, accessible from @ %lx\n", elf->elf_entry, entry);
    return entry;
}

void load_hal_and_kernel()
{
    struct loader_args *largs = get_loader_args();

    // Load HAL
    largs->hal_entry = (void *)load_elf("tdlrhal.elf",
        &largs->hal_start, &largs->hal_end);

    kprintf("HAL @ %lx to %lx, entry @ %p\n",
            largs->hal_start, largs->hal_end, largs->hal_entry);

    // Load kernel
    largs->kernel_entry = (void *)load_elf("tdlrkrnl.elf",
        &largs->kernel_start, &largs->kernel_end);

    kprintf("Kernel @ %lx to %lx, entry @ %p\n",
            largs->kernel_start, largs->kernel_end, largs->kernel_entry);

    // Set up sysarea area size
    largs->sysarea_lower = largs->hal_start < largs->kernel_start ?
                                largs->hal_start : largs->kernel_start;
    largs->sysarea_upper = largs->hal_end > largs->kernel_end ?
                                largs->hal_end : largs->kernel_end;

    largs->sysarea_lower = align_down_vaddr(largs->sysarea_lower, PAGE_SIZE);
    largs->sysarea_upper = align_up_vaddr(largs->sysarea_upper, PAGE_SIZE);

    largs->sysarea_grows_up = 1;
}
