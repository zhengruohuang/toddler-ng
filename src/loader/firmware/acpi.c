#include "common/include/inttypes.h"
#include "common/include/names.h"
#include "loader/include/devtree.h"
#include "loader/include/kprintf.h"
#include "loader/include/lib.h"
#include "loader/include/mem.h"
#include "loader/include/firmware.h"
#include "loader/include/acpi.h"


/*
 * RSDP
 */
struct acpi_rsdp {
    char signature[8];
    u8 checksum;
    char oem_id[6];
    u8 revision;
    u32 rsdt;

    // ACPI 2.0 and above
    u32 length;
    u64 xsdt;
    u8 checksum_ext;
    u8 reserved[3];
} packed_struct;


/*
 * Common table header
 */
struct acpi_header {
    char signature[4];
    u32 length;
    u8 revision;
    u8 checksum;
    char oem_id[6];
    char oem_table_id[8];
    u32 oem_revision;
    u32 creator_id;
    u32 creator_revision;
} packed_struct;


/*
 * RSDT and XSDT
 */
struct acpi_rsdt {
    struct acpi_header header;
    u32 tables[];
} packed_struct;

struct acpi_xsdt {
    struct acpi_header header;
    u64 tables[];
} packed_struct;


/*
 * MADT
 */
enum acpi_madt_table_type {
    ACPI_MADT_LAPIC = 0,
    ACPI_MADT_IOAPIC = 1,
    ACPI_MADT_INT_SRC = 2,
    ACPI_MADT_NMI = 4,
    ACPI_MADT_LAPIC_ADDR = 5,
};

struct acpi_madt {
    struct acpi_header header;
    u32 lapic_base;
    u32 flags; // 1 = Dual 8259 Legacy PICs Installed
    u8 content[];
} packed_struct;

struct acpi_madt_header {
    u8 type;
    u8 length;
} packed_struct;

struct acpi_madt_lapic {
    struct acpi_madt_header header; // type 0
    u8 acpi_processor_id;
    u8 apic_id;
    u32 flags; // bit 0 = Processor Enabled, bit 1 = Online Capable
} packed_struct;

struct acpi_madt_ioapic {
    struct acpi_madt_header header; // type 1
    u8 ioapic_id;
    u8 reserved;
    u32 ioapic_base;
    u32 global_sys_int_base;
} packed_struct;

struct acpi_madt_int_src {
    struct acpi_madt_header header; // type 2
    u8 bus_src;
    u8 irq_src;
    u32 global_sys_int;
    u16 flags;
} packed_struct;

struct acpi_madt_nmi {
    struct acpi_madt_header header; // type 4
    u8 acpi_processor_id; // 0xff == all processors
    u16 flags;
    u8 lint; // 0 or 1
} packed_struct;

struct acpi_madt_lapic_addr {
    struct acpi_madt_header header; // type 5
    u64 lapic_base64;
} packed_struct;


/*
 * Tables
 */
static struct acpi_rsdt *rsdt = NULL;
static struct acpi_xsdt *xsdt = NULL;
static struct acpi_madt *madt = NULL;

static struct acpi_header *get_table(int idx)
{
    u64 addr = 0;

    if (xsdt) {
        int count = (xsdt->header.length - sizeof(struct acpi_header)) / sizeof(u64);
        if (idx >= count) {
            return NULL;
        }
        addr = xsdt->tables[idx];

    } else {
        panic_if(!rsdt, "ACPI invalid!\n");
        int count = (rsdt->header.length - sizeof(struct acpi_header)) / sizeof(u32);
        if (idx >= count) {
            return NULL;
        }
        addr = rsdt->tables[idx];
    }

    return (void *)cast_u64_to_vaddr(addr);
}

static void *find_table(char *name)
{
    struct acpi_header *head = get_table(0);
    for (int i = 0; head; head = get_table(++i)) {
        if (!memcmp(head->signature, name, 4)) {
            return (void *)head;
        }
    }

    return NULL;
}

static void *get_madt_table(int idx)
{
    int count = 0;
    int offset = 0;
    int size = sizeof(struct acpi_header) + sizeof(u32) * 2;

    while (size < madt->header.length) {
        struct acpi_madt_header *mh = (void *)&madt->content[offset];
        if (count == idx) {
            return mh;
        }

        int mt_len = mh->length;
        offset += mt_len;
        size += mt_len;
        count++;
    }

    return NULL;
}


/*
 * MADT interface
 */
#define foreach_madt_table(head) \
    for (struct acpi_madt_header *head = get_madt_table(0); head; head = NULL) \
            for (int __i = 0; head; head = get_madt_table(++__i))

static u64 acpi_get_lapic_base()
{
    u64 lapic_base = madt->lapic_base;

    foreach_madt_table(header) {
        if (header->type == ACPI_MADT_LAPIC_ADDR) {
            struct acpi_madt_lapic_addr *table_lapic_addr = (void *)header;
            lapic_base = table_lapic_addr->lapic_base64;
            break;
        }
    }

    return lapic_base;
}

static struct acpi_madt_lapic *acpi_get_usable_lapic(int idx)
{
    int count = 0;
    foreach_madt_table(header) {
        if (header->type == ACPI_MADT_LAPIC) {
            if (count == idx) {
                return (void *)header;
            }
            count++;
        }
    }
    return NULL;
}

static int acpi_get_usable_lapic_count()
{
    int count = 0;
    foreach_madt_table(header) {
        if (header->type == ACPI_MADT_LAPIC) {
            count++;
        }
    }
    return count;
}

static struct acpi_madt_ioapic *acpi_get_ioapic(int idx)
{
    int count = 0;
    foreach_madt_table(header) {
        if (header->type == ACPI_MADT_IOAPIC) {
            if (count == idx) {
                return (void *)header;
            }
            count++;
        }
    }
    return NULL;
}

static int acpi_get_ioapic_count()
{
    int count = 0;
    foreach_madt_table(header) {
        if (header->type == ACPI_MADT_IOAPIC) {
            count++;
        }
    }
    return count;
}



/*
 * Init
 */
static void setup_lapic()
{
    // Create local APIC devtree node if needed
    struct devtree_node *node = devtree_walk("/local_apic");
    if (!node) {
        node = devtree_alloc_node(devtree_get_root(), "local_apic");
        devtree_alloc_prop_str(node, "compatible", "intel,local-apic");
    }

    // Setup prop
    u64 base = acpi_get_lapic_base();
    devtree_alloc_prop_u64(node, "acpi,base", base);
}

static void setup_ioapic()
{
    // Create IO APIC devtree node if needed
    struct devtree_node *node = devtree_walk("/io_apic");
    if (!node) {
        node = devtree_alloc_node(devtree_get_root(), "io_apic");
        devtree_alloc_prop_str(node, "compatible", "intel,io-apic");
    }

    // Setup prop
    int count = acpi_get_ioapic_count();
    int entry_size = 4 + 4 + 8; // ID, Int Base, Addr
    int prop_size = count * entry_size;

    devtree_alloc_prop_u32(node, "acpi,chip-count", count);
    struct devtree_prop *cfg = devtree_alloc_prop(node, "acpi,chip-config", NULL, prop_size);

    void *cfg_data = devtree_get_prop_data(cfg);
    for (int i = 0, offset = 0; i < count; i++, offset += entry_size) {
        struct acpi_madt_ioapic *mt = acpi_get_ioapic(i);
        panic_if(!mt, "Inconsistent IO APIC count!\n");

        *(u32 *)(cfg_data + offset + 0) = swap_big_endian32(mt->ioapic_id);
        *(u32 *)(cfg_data + offset + 4) = swap_big_endian32(mt->global_sys_int_base);
        *(u64 *)(cfg_data + offset + 8) = swap_big_endian64(mt->ioapic_base);
    }
}

static void setup_cpus()
{
    // Create CPU devtree node if needed
    struct devtree_node *node = devtree_walk("/cpus");
    if (!node) {
        node = devtree_alloc_node(devtree_get_root(), "cpus");
        devtree_alloc_prop_str(node, "compatible", ARCH_NAME);
    }

    int count = acpi_get_usable_lapic_count();
    int entry_size = 4 + 4; // Proc ID, APIC ID
    int prop_size = count * entry_size;

    devtree_alloc_prop_u32(node, "acpi,cpu-count", count);
    struct devtree_prop *cfg = devtree_alloc_prop(node, "acpi,cpu-config", NULL, prop_size);

    void *cfg_data = devtree_get_prop_data(cfg);
    for (int i = 0, offset = 0; i < count; i++, offset += entry_size) {
        struct acpi_madt_lapic *mt = acpi_get_usable_lapic(i);
        panic_if(!mt, "Inconsistent local APIC count!\n");

        *(u32 *)(cfg_data + offset + 0) = swap_big_endian32(mt->acpi_processor_id);
        *(u32 *)(cfg_data + offset + 4) = swap_big_endian32(mt->apic_id);
    }
}

// void init_acpi(struct acpi_rsdp *rsdp)
// {
//     // RSDT and XSDT
//     struct devtree_node *acpi_node = devtree_walk("/acpi");
//     if (!acpi_node) {
//         return;
//     }
//
//     struct devtree_prop *prop_rsdt = devtree_find_prop(acpi_node, "rsdt");
//     if (prop_rsdt) {
//         u32 addr = devtree_get_prop_data_u32(prop_rsdt);
//         rsdt = (void *)(ulong)addr; // TODO: explicit func cast u32 to ulong
//     }
//
//     struct devtree_prop *prop_xsdt = devtree_find_prop(acpi_node, "xsdt");
//     if (prop_xsdt) {
//         u64 addr = devtree_get_prop_data_u64(prop_xsdt);
//         xsdt = (void *)cast_u64_to_vaddr(addr);
//     }
//
//     if (!rsdt && !xsdt) {
//         return;
//     }
//
//     // MADT
//     madt = find_table("APIC");
//     if (!madt) {
//         return;
//     }
//
//     // Setup
//     setup_lapic();
//     setup_ioapic();
//     setup_cpus();
//
//     // Done
//     acpi_inited = 1;
//     kprintf("ACPI initialized, RSDT @ %p, XSDT @ %p, MADT @ %p\n", rsdt, xsdt, madt);
// }

void init_acpi(void *rsdp_ptr)
{
    struct acpi_rsdp *rsdp = rsdp_ptr;

    // Create ACPI devtree node if needed
    struct devtree_node *acpi_node = devtree_walk("/acpi");
    if (!acpi_node) {
        acpi_node = devtree_alloc_node(devtree_get_root(), "acpi");
        devtree_alloc_prop_str(acpi_node, "compatible", "intel,acpi");
    }

    // Find RSDT and XSDT
    if (!rsdp) {
        devtree_alloc_prop_str(acpi_node, "status", "disabled");
        return;
    }

    devtree_alloc_prop_u32(acpi_node, "rsdt", rsdp->rsdt);
    if (rsdp->revision) {
        devtree_alloc_prop_u64(acpi_node, "xsdt", rsdp->xsdt);
    }

    rsdt = (void *)(ulong)rsdp->rsdt; // TODO: explicit func cast u32 to ulong
    xsdt = (void *)cast_u64_to_vaddr(rsdp->xsdt);
    if (!rsdt && !xsdt) {
        return;
    }

    // MADT
    madt = find_table("APIC");
    if (!madt) {
        return;
    }

    // Setup
    setup_lapic();
    setup_ioapic();
    setup_cpus();

    // Done
    kprintf("ACPI initialized, RSDT @ %p, XSDT @ %p, MADT @ %p\n", rsdt, xsdt, madt);
}

void *find_acpi_rsdp(ulong start, ulong end)
{
    start = align_up_vaddr(start, 16);
    end = align_down_vaddr(end, 16);

    for (ulong addr = start; addr < end; addr += 16) {
        void *signature = (void *)addr;
        if (!memcmp(signature, "RSD PTR ", 8)) {
            return signature;
        }
    }

    return NULL;
}
