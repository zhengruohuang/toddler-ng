#include "common/include/inttypes.h"
#include "loader/include/lprintf.h"
#include "loader/include/firmware.h"


static u8 io_read8(void *addr)
{
    return 0;
//     volatile u8 *ptr = (u8 *)(SEG_DIRECT + addr);
//     return *ptr;
}

static void io_write8(void *addr, u8 val)
{
//     volatile u8 *ptr = (u8 *)(SEG_DIRECT + addr);
//     *ptr = val;
}


static void *ns16550_base = NULL;
static int ns16550_size = 0;
static int reg_shift = 0;

#define DATA_ADDR           (ns16550_base + (0x0 << reg_shift))
#define INT_ENABLE_ADDR     (ns16550_base + (0x8 << reg_shift))
#define INT_ID_FIFO_ADDR    (ns16550_base + (0x10 << reg_shift))
#define LINE_CTRL_ADDR      (ns16550_base + (0x18 << reg_shift))
#define MODEM_CTRL_ADDR     (ns16550_base + (0x20 << reg_shift))
#define LINE_STAT_ADDR      (ns16550_base + (0x28 << reg_shift))
#define MODEM_STAT_ADDR     (ns16550_base + (0x30 << reg_shift))
#define SCRATCH_ADDR        (ns16550_base + (0x38 << reg_shift))

#define DIV_LSB_ADDR        (ns16550_base + (0x0 << reg_shift))
#define DIV_MSB_ADDR        (ns16550_base + (0x8 << reg_shift))

#define MAX_BAUD            1152000


static int putchar(int ch)
{
    u8 ready = 0;
    while (!ready) {
        ready = io_read8(LINE_STAT_ADDR) & 0x20;
    }

    io_write8(DATA_ADDR, (u8)(ch & 0xff));

    return ch & 0xff;
}

static void resolve_opts(const char *opts,
    int *data, int *parity, int *stop, int *baud)
{
    if (!opts || !opts[0]) {
        return;
    }

    // Part: 0 - baud, 1 - parity, 2 - bits, 3 - flow, 4 - done
    int cur_part = 0;
    int cur_baud = 0, cur_parity = 0, cur_bits = 0, cur_flow = 0;

    while (*opts && cur_part != 4) {
        char ch = *opts;
        switch (ch) {
        case 'n': cur_part = 2; cur_parity = 0; break;
        case 'o': cur_part = 2; cur_parity = 1; break;
        case 'e': cur_part = 2; cur_parity = 2; break;
        case 'r': cur_part = 4; cur_flow = 1; break;
        default:
            if (ch >= '0' && ch <= '9') {
                if (cur_part == 0) cur_baud = cur_baud * 10 + ch - '0';
                else if (cur_part == 2) cur_bits = cur_bits * 10 + ch - '0';
            }
            break;
        }

        opts++;
    }

    if (data) *data = cur_bits;
    if (parity) *parity = cur_parity;
    if (stop) *stop = cur_flow;
    if (baud) *baud = cur_baud;
}

static void setup(const void *node, const char *opts)
{
//     // Find reg shift
//     struct fdt_struct_prop *prop =
//         fdt_get_prop((struct fdt_struct *)node, "reg-shift");
//     if (prop) {
//         reg_shift = fdt_prop_parse_int(prop);
//     }
//
//     // Find freq
//     int freq = MAX_BAUD;
//     prop = fdt_get_prop((struct fdt_struct *)node, "clock-frequency");
//     if (prop) {
//         freq = fdt_prop_parse_int(prop);
//     }
//
//     // Find out default baud
//     int baud = 9600;
//     prop = fdt_get_prop((struct fdt_struct *)node, "current-speed");
//     if (prop) {
//         baud = fdt_prop_parse_int(prop);
//     }
//
//     // Resolve opts, default 8-bit data, no parity, 1-bit stop
//     int data = 0, parity = 0, stop = 0;
//     resolve_opts(opts, &data, &parity, &stop, &baud);
//
//     // Disable interrupts
//     io_write8(INT_ENABLE_ADDR, 0);
//
//     // Set up buad rate
//     int divisor = freq / baud;
//     io_write8(LINE_CTRL_ADDR, 0x80);
//     io_write8(DIV_LSB_ADDR, divisor & 0xff);
//     io_write8(DIV_MSB_ADDR, (divisor >> 8) & 0xff);
//     io_write8(LINE_CTRL_ADDR, 0x0);
//
//     // Set data format
//     io_write8(LINE_CTRL_ADDR, data | parity | stop);
}

static int find_base(const void *node)
{
//     u64 addr = 0, size = 0;
//     int idx = fdt_get_reg((struct fdt_struct *)node, 0, &addr, &size);
//     if (idx != -1) {
//         // Translate
//         addr = fdt_translate_reg((struct fdt_struct *)node, addr);
//
//         // Done
//         ns16550_base = (void *)(ulong)addr;
//         ns16550_size = (int)size;
//         return 0;
//     }

    return -1;
}


int init_ns16550(const void *node, const char *opts)
{
    // Find out where the device is
    if (!find_base(node)) {
        return -1;
    }

    // Set up the device
    setup(node, opts);

    // Register putchar
    init_libk_putchar(putchar);

    return 0;
}
