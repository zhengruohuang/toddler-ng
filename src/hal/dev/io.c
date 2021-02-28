#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "hal/include/hal.h"


static inline void mmio_mb()
{
    atomic_mb();
}


/*
 * MMIO
 */
u8 mmio_read8(ulong addr)
{
    volatile u8 *ptr = (u8 *)addr;
    u8 val = 0;

    mmio_mb();
    val = *ptr;
    mmio_mb();

    return val;
}

void mmio_write8(ulong addr, u8 val)
{
    volatile u8 *ptr = (u8 *)addr;

    mmio_mb();
    *ptr = val;
    mmio_mb();
}

u16 mmio_read16(ulong addr)
{
    volatile u16 *ptr = (u16 *)addr;
    u16 val = 0;

    mmio_mb();
    val = *ptr;
    mmio_mb();

    return val;
}

void mmio_write16(ulong addr, u16 val)
{
    volatile u16 *ptr = (u16 *)addr;

    mmio_mb();
    *ptr = val;
    mmio_mb();
}

u32 mmio_read32(ulong addr)
{
    volatile u32 *ptr = (u32 *)addr;
    u32 val = 0;

    mmio_mb();
    val = *ptr;
    mmio_mb();

    return val;
}

void mmio_write32(ulong addr, u32 val)
{
    volatile u32 *ptr = (u32 *)addr;

    mmio_mb();
    *ptr = val;
    mmio_mb();
}


/*
 * Port IO
 */
u8 port_read8(ulong addr)
{
    return arch_hal_has_io_port() ?
            (u8)arch_hal_ioport_read(addr, 1) : mmio_read8(addr);
}

void port_write8(ulong addr, u8 val)
{
    arch_hal_has_io_port() ?
            arch_hal_ioport_write(addr, 1, val) : mmio_write8(addr, val);
}

u16 port_read16(ulong addr)
{
    return arch_hal_has_io_port() ?
            (u16)arch_hal_ioport_read(addr, 2) : mmio_read16(addr);
}

void port_write16(ulong addr, u16 val)
{
    arch_hal_has_io_port() ?
            arch_hal_ioport_write(addr, 2, val) : mmio_write16(addr, val);
}

u32 port_read32(ulong addr)
{
    return arch_hal_has_io_port() ?
            (u32)arch_hal_ioport_read(addr, 4) : mmio_read32(addr);
}

void port_write32(ulong addr, u32 val)
{
    arch_hal_has_io_port() ?
            arch_hal_ioport_write(addr, 4, val) : mmio_write32(addr, val);
}


/*
 * 64-bit IO
 */
#if (ARCH_WIDTH == 64)
u64 mmio_read64(ulong addr)
{
    volatile u64 *ptr = (u64 *)addr;
    u64 val = 0;

    mmio_mb();
    val = *ptr;
    mmio_mb();

    return val;
}

void mmio_write64(ulong addr, u64 val)
{
    volatile u64 *ptr = (u64 *)addr;

    mmio_mb();
    *ptr = val;
    mmio_mb();
}

u64 port_read64(ulong addr)
{
    return arch_hal_has_io_port() ?
            (u64)arch_hal_ioport_read(addr, 8) : mmio_read64(addr);
}

void port_write64(ulong addr, u64 val)
{
    arch_hal_has_io_port() ?
            arch_hal_ioport_write(addr, 8, val) : mmio_write64(addr, val);
}
#endif
