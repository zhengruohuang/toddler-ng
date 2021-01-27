#include <stdio.h>
#include <atomic.h>
#include <sys.h>
#include <kth.h>
#include <drv/drv.h>

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

static shiftbuf_t pl011_shiftbuf = { .value = 0 };

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
    } while (!atomic_cas(&sb->value, old_val.value, new_val.value));

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
    } while (!atomic_cas(&sb->value, old_val.value, new_val.value));

    if (data) {
        *data = byte;
    }

    return 1;
}



/*
 * PL011
 */
#define BCM2835_BASE            (0x3f000000ul)
#define PL011_BASE              (BCM2835_BASE + 0x201000ul)

struct pl011_mmio {
    u32 DR;     // Data register
    u32 RSRECR; // Receive status register/error clear register
    u32 PAD[4];
    u32 FR;     // Flag register
    u32 RES1;   // Reserved
    u32 ILPR;   // Not in use
    u32 IBRD;   // Integer baud rate divisor
    u32 FBRD;   // Fractional baud rate divisor
    u32 LCRH;   // Line control register
    u32 CR;     // Control register
    u32 IFLS;   // Interrupt FIFO level select register
    u32 IMSC;   // Interrupt mask set clear register
    u32 RIS;    // Raw interrupt status register
    u32 MIS;    // Masked interrupt status register
    u32 ICR;    // Interrupt clear register
    u32 DMACR;  // DMA control register
};

struct pl011_driver {
    unsigned long seq;
    volatile struct pl011_mmio *mmio;
};

static struct pl011_driver pl011;

static inline int pl011_received()
{
    return !(pl011.mmio->FR & 0x10);
}

static inline int pl011_read_one()
{
    // Wait until the UART isn't busy
    while (pl011.mmio->FR & 0x8);

    // Return the data
    return (int)pl011.mmio->DR;
}

static inline void pl011_write_one(char ch)
{
    // Wait until the UART has an empty space in the FIFO
    while (pl011.mmio->FR & 0x20);

    // Write the character to the FIFO for transmission
    pl011.mmio->DR = ch;
}


/*
 * Interrupt
 */
static sema_t pl011_sema = SEMA_INITIALIZER(sizeof(unsigned long) - 1);

static void pl011_int_handler(unsigned long param)
{
    //kprintf("PL011 int handler!\n");

    // Disable all interrupts
    pl011.mmio->IMSC = 0;

    // Clear all interrupt status
    pl011.mmio->ICR = 0x7FF;

    // Read all
    int rc = 0;
    while (pl011_received()) {
        int c = pl011_read_one();
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
        shiftbuf_write_one(&pl011_shiftbuf, c);
        rc++;
    }

    // Enable receiving interrupt
    pl011.mmio->IMSC = 0x10;

    // EOI
    syscall_int_eoi(pl011.seq);

//     // Test
//     pl011_write_one('a');
//     pl011_write_one('b');
//     pl011_write_one('\n');

    // Wake up readers
    if (rc) {
        sema_post_count(&pl011_sema, rc);
    }

    // Done
    syscall_thread_exit_self(0);
}


/*
 * Driver ops
 */
static int dev_pl011_read(devid_t id, void *buf, size_t count, size_t offset,
                          struct fs_file_op_result *result)
{
    size_t read_count = 0;
    unsigned char *bytes = buf;

    do {
        sema_wait(&pl011_sema);

        while (count) {
            unsigned char byte = 'a';
            int r = shiftbuf_read_one(&pl011_shiftbuf, &byte);
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

static int dev_pl011_write(devid_t id, void *buf, size_t count, size_t offset,
                           struct fs_file_op_result *result)
{
    if (buf && count) {
        unsigned char *array = buf;
        for (size_t i = 0; i < count; i++) {
            pl011_write_one(array[i]);
        }
    }

    result->count = count;
    return 0;
}


/*
 * Init
 */
static const struct drv_ops dev_pl011_ops = {
    .read = dev_pl011_read,
    .write = dev_pl011_write,
};

static inline void start_pl011()
{
    // Map PL011 registers
    unsigned long vbase = syscall_vm_map(VM_MAP_DEV, paddr_to_ppfn(PL011_BASE), PAGE_SIZE);
    unsigned long offset = PL011_BASE - ppfn_to_paddr(paddr_to_ppfn(PL011_BASE));
    pl011.mmio = (void *)(vbase + offset);

    // Clear the receiving FIFO
    while (!(pl011.mmio->FR & 0x10)) {
        (void)pl011.mmio->DR;
    }

    // Enable receiving interrupt and disable irrevelent ones
    pl011.mmio->IMSC = 0x10;

    // Clear all interrupt status
    pl011.mmio->ICR = 0x7FF;
}

void init_pl011_driver()
{
    // Register handler
    pl011.seq = syscall_int_handler(0x18, pl011_int_handler);

    // Register driver
    create_drv("/dev", "serial", 0, &dev_pl011_ops, 1);

    // Start
    start_pl011();
}
