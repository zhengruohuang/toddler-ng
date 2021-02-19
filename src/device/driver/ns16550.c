#include <stdio.h>
#include <atomic.h>
#include <sys.h>
#include <kth.h>
#include <drv/drv.h>

#include "common/include/io.h"
#include "libk/include/mem.h"


/*
 * Shift buffer
 */
typedef volatile struct shiftbuf {
    union {
        unsigned long value;

        struct {
            unsigned long count : 8;
            unsigned long data  : sizeof(unsigned long) * 8 - 8;
        };
    };
} shiftbuf_t;

static shiftbuf_t ns16550_shiftbuf = { .value = 0 };

static int shiftbuf_write_one(shiftbuf_t *sb, unsigned char data)
{
    shiftbuf_t old_val, new_val;

    do {
        old_val.value = sb->value;

        new_val.data = old_val.data << 8;
        new_val.data |= data;
        new_val.count = old_val.count + 1;
        if (new_val.count > sizeof(unsigned long) - 1) {
            new_val.count = sizeof(unsigned long) - 1;
        }
    } while (!atomic_cas_bool(&sb->value, old_val.value, new_val.value));

    return 1;
}

static int shiftbuf_read_one(shiftbuf_t *sb, unsigned char *data)
{
    if (!sb->count) {
        return 0;
    }

    shiftbuf_t old_val, new_val;
    unsigned char byte = 0;

    do {
        old_val.value = sb->value;
        if (!old_val.count) {
            return 0;
        }

        byte = (old_val.data >> ((old_val.count - 1) * 8)) & 0xfful;

        new_val.value = old_val.value;
        new_val.count--;
    } while (!atomic_cas_bool(&sb->value, old_val.value, new_val.value));

    if (data) {
        *data = byte;
    }

    return 1;
}



/*
 * NS16550
 */
#define SOUTH_BRIDGE_BASE_ADDR  0x18000000
#define UART_BASE_ADDR          (SOUTH_BRIDGE_BASE_ADDR + 0x3f8)
#define UART_DATA_REG           (0x0)
#define UART_LINE_STAT_REG      (0x28)

struct ns16550_driver {
    unsigned long seq;
    unsigned long mmio_base;
};

static struct ns16550_driver ns16550;

static inline int ns16550_received()
{
    return 0;
}

static inline int ns16550_read_one()
{
    return 0;
}

static inline u8 __mmio_read8(ulong addr)
{
    volatile u8 *ptr = (u8 *)addr;
    u8 val = 0;

    mmio_mb();
    val = *ptr;
    mmio_mb();

    return val;
}

static inline void __mmio_write8(ulong addr, u8 val)
{
    volatile u8 *ptr = (u8 *)addr;

    mmio_mb();
    *ptr = val;
    mmio_mb();
}

static inline void ns16550_write_one(char ch)
{
    u32 ready = 0;
    while (!ready) {
        ready = __mmio_read8(ns16550.mmio_base + UART_LINE_STAT_REG) & 0x20;
    }

    __mmio_write8(ns16550.mmio_base + UART_DATA_REG, (u8)ch & 0xff);
}


/*
 * Interrupt
 */
static sema_t ns16550_sema = SEMA_INITIALIZER(sizeof(unsigned long) - 1);

static void ns16550_int_handler(unsigned long param)
{
    //kprintf("NS16550 int handler!\n");

    // Disable all interrupts
    //ns16550.mmio->IMSC = 0;

    // Clear all interrupt status
    //ns16550.mmio->ICR = 0x7FF;

    // Read all
    int rc = 0;
    while (ns16550_received()) {
        int c = ns16550_read_one();
        switch (c) {
        case 0x7f:
            c = '\b';
            break;
        case '\r':
            c = '\n';
            break;
        default:
            break;
        }

        //kprintf("PL011 char: %c\n", c);
        shiftbuf_write_one(&ns16550_shiftbuf, c);
        rc++;
    }

    // Enable receiving interrupt
    //ns16550.mmio->IMSC = 0x10;

    // EOI
    syscall_int_eoi(ns16550.seq);

//     // Test
//     ns16550_write_one('a');
//     ns16550_write_one('b');
//     ns16550_write_one('\n');

    // Wake up readers
    if (rc) {
        sema_post_count(&ns16550_sema, rc);
    }

    // Done
    syscall_thread_exit_self(0);
}


/*
 * Driver ops
 */
static int dev_ns16550_read(devid_t id, void *buf, size_t count, size_t offset,
                          struct fs_file_op_result *result)
{
    size_t read_count = 0;
    unsigned char *bytes = buf;

    do {
        sema_wait(&ns16550_sema);

        while (count) {
            unsigned char byte = 'a';
            int r = shiftbuf_read_one(&ns16550_shiftbuf, &byte);
            if (!r) {
                break;
            }

            if (bytes) {
                *bytes = byte;
                bytes++;
            }

            read_count++;
            count--;
        }
    } while (!read_count);

    result->count = read_count;
    result->more = 0;
    return 0;
}

static int dev_ns16550_write(devid_t id, void *buf, size_t count, size_t offset,
                           struct fs_file_op_result *result)
{
    if (buf && count) {
        unsigned char *array = buf;
        for (size_t i = 0; i < count; i++) {
            ns16550_write_one(array[i]);
        }
    }

    result->count = count;
    return 0;
}


/*
 * Init
 */
static const struct drv_ops dev_ns16550_ops = {
    .read = dev_ns16550_read,
    .write = dev_ns16550_write,
};

static inline void start_ns16550()
{
    // Map NS16550 registers
    unsigned long vbase = syscall_vm_map(VM_MAP_DEV, paddr_to_ppfn(UART_BASE_ADDR), PAGE_SIZE);
    unsigned long offset = UART_BASE_ADDR - ppfn_to_paddr(paddr_to_ppfn(UART_BASE_ADDR));
    ns16550.mmio_base = vbase + offset;
}

void init_ns16550_driver()
{
    // Register handler
    //ns16550.seq = syscall_int_handler(0x18, ns16550_int_handler);

    // Register driver
    create_drv("/dev", "serial", 0, &dev_ns16550_ops, 1);

    // Start
    start_ns16550();
}
