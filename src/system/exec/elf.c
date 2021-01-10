#include <stdio.h>
#include <string.h>
#include <sys.h>
#include "common/include/abi.h"
#include "common/include/elf.h"
#include "libk/include/mem.h"


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


static int load_elf(pid_t pid, FILE *f, unsigned long *entry,
                    unsigned long *vstart, unsigned long *vend)
{
    unsigned long vrange_start = -1ul, vrange_end = 0;

    elf_native_header_t elf;
    fread(&elf, 1, sizeof(elf_native_header_t), f);

    if (entry) *entry = elf.elf_entry;
    kprintf("\tELF entry @ %lx, phnum: %u\n", (unsigned long)elf.elf_entry, elf.elf_phnum);

    for (int i = 0, phoff = elf.elf_phoff; i < elf.elf_phnum;
         i++, phoff += elf.elf_phentsize
    ) {
        kprintf("\t\tSegment #%d\n", i);

        elf_native_program_t prog;
        fseek(f, phoff, SEEK_SET);
        fread(&prog, 1, sizeof(elf_native_program_t), f);
        kprintf("\t\tfilesz: %u, memsz: %u, vaddr: %lx, offset: %lx\n",
                prog.program_filesz, prog.program_memsz,
                (unsigned long)prog.program_vaddr, (unsigned long)prog.program_offset);

        unsigned long remote_vaddr = align_down_vaddr(prog.program_vaddr, PAGE_SIZE);
        unsigned long remote_vend = align_up_vaddr(prog.program_vaddr + prog.program_memsz, PAGE_SIZE);
        if (remote_vaddr < vrange_start) {
            vrange_start = remote_vaddr;
        }
        if (remote_vend > vrange_end) {
            vrange_end = remote_vend;
        }

        if (!prog.program_memsz) {
            continue;
        }
        unsigned long map_size = remote_vend - remote_vaddr;
        unsigned long local_vaddr = syscall_vm_map_cross(pid, remote_vaddr, map_size, 0);
        //kprintf("\t\tlocal vaddr @ %lx\n", local_vaddr);
        memzero((void *)local_vaddr, map_size);

        if (!prog.program_filesz) {
            continue;
        }
        unsigned long copy_vaddr = local_vaddr + (prog.program_vaddr - remote_vaddr);
        fseek(f, prog.program_offset, SEEK_SET);
        fread((void *)copy_vaddr, 1, prog.program_filesz, f);

        //kprintf("Copy vaddr @ %lx: %lx, prog offset: %u\n", copy_vaddr, *(unsigned long *)copy_vaddr, prog.program_offset);
    }

    if (vstart) *vstart = vrange_start;
    if (vend) *vend = vrange_end;

    //kprintf("copy done!\n");

    return 0;
}

int exec_elf(pid_t pid, const char *path, unsigned long *entry, unsigned long *free_start)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return -1;
    }

    unsigned long vstart = 0, vend = 0;
    int err = load_elf(pid, f, entry, &vstart, &vend);
    fclose(f);

    if (free_start) {
        *free_start = vend;
    }

    return err;
}
