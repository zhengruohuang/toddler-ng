#ifndef __ARCH_MIPS_COMMON_INCLUDE_IO__
#define __ARCH_MIPS_COMMON_INCLUDE_IO__


#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "common/include/atomic.h"


static inline void mmio_mb()
{
    atomic_mb();
}


static inline u8 mmio_read8(unsigned long addr)
{
    volatile u8 *ptr = (u8 *)(SEG_ACC_DIRECT | addr);
    u8 val = 0;

    mmio_mb();
    val = *ptr;
    mmio_mb();

    return val;
}

static inline void mmio_write8(unsigned long addr, u8 val)
{
    volatile u8 *ptr = (u8 *)(SEG_ACC_DIRECT | addr);

    mmio_mb();
    *ptr = val;
    mmio_mb();
}

static inline u16 mmio_read16(unsigned long addr)
{
    volatile u16 *ptr = (u16 *)(SEG_ACC_DIRECT | addr);
    u16 val = 0;

    mmio_mb();
    val = *ptr;
    mmio_mb();

    return val;
}

static inline void mmio_write16(unsigned long addr, u16 val)
{
    volatile u16 *ptr = (u16 *)(SEG_ACC_DIRECT | addr);

    mmio_mb();
    *ptr = val;
    mmio_mb();
}

static inline u32 mmio_read32(unsigned long addr)
{
    volatile u32 *ptr = (u32 *)(SEG_ACC_DIRECT | addr);
    u32 val = 0;

    mmio_mb();
    val = *ptr;
    mmio_mb();

    return val;
}

static inline void mmio_write32(unsigned long addr, u32 val)
{
    volatile u32 *ptr = (u32 *)(SEG_ACC_DIRECT | addr);

    mmio_mb();
    *ptr = val;
    mmio_mb();
}

static inline u64 mmio_read64(unsigned long addr)
{
    volatile u64 *ptr = (u64 *)(SEG_ACC_DIRECT | addr);
    u64 val = 0;

    mmio_mb();
    val = *ptr;
    mmio_mb();

    return val;
}

static inline void mmio_write64(unsigned long addr, u64 val)
{
    volatile u64 *ptr = (u64 *)(SEG_ACC_DIRECT | addr);

    mmio_mb();
    *ptr = val;
    mmio_mb();
}


static inline u8 ioport_read8(unsigned long addr)
    { return mmio_read8(addr); }
static inline void ioport_write8(unsigned long addr, u8 val)
    { mmio_write8(addr, val); }

static inline u16 ioport_read16(unsigned long addr)
    { return mmio_read16(addr); }
static inline void ioport_write16(unsigned long addr, u16 val)
    { mmio_write16(addr, val); }

static inline u32 ioport_read32(unsigned long addr)
    { return mmio_read32(addr); }
static inline void ioport_write32(unsigned long addr, u32 val)
    { mmio_write32(addr, val); }

static inline u64 ioport_read64(unsigned long addr)
    { return mmio_read64(addr); }
static inline void ioport_write64(unsigned long addr, u64 val)
    { mmio_write64(addr, val); }


#endif
