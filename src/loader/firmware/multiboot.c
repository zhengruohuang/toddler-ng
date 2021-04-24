#include "common/include/inttypes.h"
#include "loader/include/lib.h"
#include "loader/include/kprintf.h"
#include "loader/include/devtree.h"
#include "loader/include/firmware.h"
#include "loader/include/multiboot.h"
#include "loader/include/acpi.h"


/*
 * Multiboot
 */
static struct multiboot_info *mbi;
static int multiboot_x86 = 0;

static struct devtree_node *get_chosen_node()
{
    struct devtree_node *chosen = devtree_walk("/chosen");
    if (!chosen) {
        chosen = devtree_alloc_node(devtree_get_root(), "chosen");
    }

    return chosen;
}

static struct devtree_node *get_memory_node()
{
    struct devtree_node *memory = devtree_walk("/memory");
    if (!memory) {
        memory = devtree_alloc_node(devtree_get_root(), "memory");
    }

    return memory;
}

static struct devtree_node *get_memrsv_node()
{
    struct devtree_node *memrsv = devtree_walk("/memrsv-block");
    if (!memrsv) {
        memrsv = devtree_alloc_node(devtree_get_root(), "memrsv-block");
    }

    return memrsv;
}

static void find_and_parse_fdt()
{
    struct multiboot_mod_list *e = (void *)(ulong)mbi->mods_addr;

    for (int i = 0; i < mbi->mods_count && e; i++, e++) {
        char *str = (void *)(ulong)e->cmdline;
        if (strstr(str, "fdt.dtb") && e->mod_start) {
            void *fdt = (void *)(ulong)e->mod_start;
            if (is_fdt_header(fdt)) {
                init_supplied_fdt(fdt);
                break;
            }
        }

        kprintf("Mod @ %x, end @ %x, str: %s\n", e->mod_start, e->mod_end, str);
    }

    if (!devtree_get_root()) {
        init_appended_fdt();
    }
}

static void find_and_parse_initrd()
{
    struct devtree_node *chosen = get_chosen_node();

    struct multiboot_mod_list *e = (void *)(ulong)mbi->mods_addr;
    for (int i = 0; i < mbi->mods_count && e; i++, e++) {
        char *str = (void *)(ulong)e->cmdline;
        if (strstr(str, "coreimg.img") && e->mod_start && e->mod_end) {
            u64 initrd_start = e->mod_start;
            u64 initrd_end = e->mod_end;

            devtree_alloc_prop_u64(chosen, "initrd-start", initrd_start);
            devtree_alloc_prop_u64(chosen, "initrd-end", initrd_end);

            kprintf("Coreimg @ %x, end @ %x, str: %s\n", e->mod_start, e->mod_end, str);
            break;
        }
    }
}

static void parse_cmd()
{
    struct devtree_node *chosen = get_chosen_node();

    char *cmd = (void *)(ulong)mbi->cmdline;
    char *bootargs_str = "";
    if (cmd) {
        bootargs_str = strchr(cmd, ' ');
        if (bootargs_str && bootargs_str[0] && bootargs_str[1]) {
            bootargs_str++;
            __unused_var struct devtree_prop *bootargs =
                devtree_alloc_prop(chosen, "bootargs", bootargs_str, strlen(bootargs_str) + 1);
        }
    }

    kprintf("Bootarg: %s\n", bootargs_str ? bootargs_str : "(empty)");
}

static void parse_mmap()
{
    u32 mem_start = mbi->mem_lower;
    u32 mem_end = mbi->mem_upper;
    kprintf("Mem start @ %x, end @ %x\n", mem_start, mem_end);

    // First find out total memory size and total number of available slots
    u64 memstart = 0;
    u64 memend = 0;
    int slots = 0;

    for (struct multiboot_mmap_entry *e = (void *)(ulong)mbi->mmap_addr;
        e && (ulong)e < (ulong)mbi->mmap_addr + (ulong)mbi->mmap_length;
        e = (void *)e + e->size + sizeof(e->size)
    ) {
        if ((e->type == MULTIBOOT_MEMORY_AVAILABLE) ||
            (e->type == MULTIBOOT_MEMORY_RESERVED && e->addr == memend)
        ) {
            if (e->addr < memstart) memstart = e->addr;
            if (e->addr + e->len > memend) memend = e->addr + e->len;
            slots++;
        }

        kprintf("MMap entry @ %p, size: %x, start @ %llx, len: %llx, type: %u\n",
            e, e->size, e->addr, e->len, e->type);
    }

    // Construct memsize node
    struct devtree_node *chosen = get_chosen_node();
    u64 memsize = memend > memstart ? memend - memstart : 0;
    if (memsize) {
        devtree_alloc_prop_u64(chosen, "memsize", memsize);
    }

    // Construct memory node
    struct devtree_node *memory = get_memory_node();

    int num_addr_cells = 0, num_size_cells = 0;
    devtree_get_reg_num_cells(memory, &num_addr_cells, &num_size_cells);

    struct devtree_prop *mem_reg_prop = NULL;
    if (num_addr_cells == 1 && num_size_cells == 1) {
        mem_reg_prop =
            devtree_alloc_prop(memory, "reg", NULL, slots * 2 * sizeof(u32));
    } else if (num_addr_cells == 2 && num_size_cells == 2) {
        mem_reg_prop =
            devtree_alloc_prop(memory, "reg", NULL, slots * 2 * sizeof(u64));
    } else {
        panic("Unsupported memory addr and size cell sizes!");
    }

    void *mem_reg_slots = devtree_get_prop_data(mem_reg_prop);
    int mem_reg_idx = 0;

    for (struct multiboot_mmap_entry *e = (void *)(ulong)mbi->mmap_addr;
        e && (ulong)e < (ulong)mbi->mmap_addr + (ulong)mbi->mmap_length;
        e = (void *)e + e->size + sizeof(e->size)
    ) {
        if (e->type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (num_addr_cells == 1) {
                u32 start = e->addr;
                u32 len = e->len;
                panic_if((u64)start != e->addr, "Mem addr out of 32-bit range!\n");
                panic_if((u64)len != e->len, "Mem addr out of 32-bit range!\n");
                ((u32 *)mem_reg_slots)[mem_reg_idx++] = swap_big_endian32(start);
                ((u32 *)mem_reg_slots)[mem_reg_idx++] = swap_big_endian32(len);

                kprintf("Avail slot @ %x, len: %x\n", start, len);
            } else if (num_addr_cells == 2) {
                u64 start = e->addr;
                u64 len = e->len;
                ((u64 *)mem_reg_slots)[mem_reg_idx++] = swap_big_endian64(start);
                ((u64 *)mem_reg_slots)[mem_reg_idx++] = swap_big_endian64(len);

                kprintf("Avail slot 64 @ %llx, len: %llx\n", start, len);
            }
        }
    }
}

static void build_reserve_node()
{
    // Reserve low 1MB on x86, otherwise reserve low 64KB,
    // for potential special needs
    u64 cell[2] = {
        0x0ull,
        swap_big_endian64(multiboot_x86 ? 0x100000ull : 0x10000ull)
    };

    struct devtree_node *memrsv = get_memrsv_node();
    devtree_alloc_prop(memrsv, "rsv0", cell, sizeof(cell));
}

static void parse_framebuffer()
{
    kprintf("Framebuffer @ %llx, pitch: %u, width: %u, height: %u, bpp: %u, type: %d\n",
        mbi->framebuffer_addr, mbi->framebuffer_pitch,
        mbi->framebuffer_width, mbi->framebuffer_height,
        mbi->framebuffer_bpp, mbi->framebuffer_type
    );
}

static void find_and_setup_x86_acpi()
{
    void *rsdp = find_acpi_rsdp(0x80000, 0xa0000);
    if (!rsdp) {
        rsdp = find_acpi_rsdp(0xe0000, 0x100000);
    }

    if (rsdp) {
        init_acpi(rsdp);
    }
}

static void init_multiboot(void *multiboot)
{
    mbi = multiboot;
    multiboot_x86 = 1;

    find_and_parse_fdt();
    find_and_parse_initrd();
    parse_cmd();
    parse_mmap();
    build_reserve_node();
    parse_framebuffer();

    if (multiboot_x86) {
        find_and_setup_x86_acpi();
    }
}

DECLARE_FIRMWARE_DRIVER(multiboot) = {
    .name = "multiboot",
    .init = init_multiboot,
};
