#include <sys.h>
#include <stdio.h>
#include "libk/include/mem.h"
#include "kernel/include/proc.h"


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

static void pl011_read_all()
{
    // Disable all interrupts
    pl011.mmio->IMSC = 0;

    // Clear all interrupt status
    pl011.mmio->ICR = 0x7FF;

    //kprintf("data: %x\n", pl011.mmio->DR);

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

        kprintf("Char: %c\n", c);
    }

    // Enable receiving interrupt
    pl011.mmio->IMSC = 0x10;

    // EOI
    syscall_int_eoi(pl011.seq);
}

static inline void pl011_write_one(char ch)
{
    // Wait until the UART has an empty space in the FIFO
    while (pl011.mmio->FR & 0x20);

    // Write the character to the FIFO for transmission
    pl011.mmio->DR = ch;
}

static void pl011_int_handler(unsigned long param)
{
    //kprintf("PL011 int handler!\n");
    pl011_read_all();
    pl011_write_one('a');
    pl011_write_one('b');
    pl011_write_one('\n');
    syscall_thread_exit_self(0);
}

void init_pl011_driver()
{
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

    // Register handler
    pl011.seq = syscall_int_handler(0x18, pl011_int_handler);
}
