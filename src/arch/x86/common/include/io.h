// #ifndef __ARCH_X86_COMMON_INCLUDE_IO_H__
// #define __ARCH_X86_COMMON_INCLUDE_IO_H__
//
//
// #include "common/include/inttypes.h"
// // #include "common/include/atomic.h"
//
//
// static inline void mmio_mb()
// {
//     //atomic_mb();
// }
//
//
// static inline u8 mmio_read8(unsigned long addr)
// {
//     volatile u8 *ptr = (u8 *)addr;
//     u8 val = 0;
//
//     mmio_mb();
//     val = *ptr;
//     mmio_mb();
//
//     return val;
// }
//
// static inline void mmio_write8(unsigned long addr, u8 val)
// {
//     volatile u8 *ptr = (u8 *)addr;
//
//     mmio_mb();
//     *ptr = val;
//     mmio_mb();
// }
//
// static inline u16 mmio_read16(unsigned long addr)
// {
//     volatile u16 *ptr = (u16 *)addr;
//     u16 val = 0;
//
//     mmio_mb();
//     val = *ptr;
//     mmio_mb();
//
//     return val;
// }
//
// static inline void mmio_write16(unsigned long addr, u16 val)
// {
//     volatile u16 *ptr = (u16 *)addr;
//
//     mmio_mb();
//     *ptr = val;
//     mmio_mb();
// }
//
// static inline u32 mmio_read32(unsigned long addr)
// {
//     volatile u32 *ptr = (u32 *)addr;
//     u32 val = 0;
//
//     mmio_mb();
//     val = *ptr;
//     mmio_mb();
//
//     return val;
// }
//
// static inline void mmio_write32(unsigned long addr, u32 val)
// {
//     volatile u32 *ptr = (u32 *)addr;
//
//     mmio_mb();
//     *ptr = val;
//     mmio_mb();
// }
//
// static inline u64 mmio_read64(unsigned long addr)
// {
//     volatile u64 *ptr = (u64 *)addr;
//     u64 val = 0;
//
//     mmio_mb();
//     val = *ptr;
//     mmio_mb();
//
//     return val;
// }
//
// static inline void mmio_write64(unsigned long addr, u64 val)
// {
//     volatile u64 *ptr = (u64 *)addr;
//
//     mmio_mb();
//     *ptr = val;
//     mmio_mb();
// }
//
//
// static inline u8 ioport_read8(u16 port)
// {
//     u8 data;
//
//     __asm__ __volatile__ (
//         "inb %[port], %[data];"
//         : [data] "=a" (data)
//         : [port] "Nd" (port)
//     );
//
//     return data;
// }
//
// static inline void ioport_write8(u16 port, u8 data)
// {
//     __asm__ __volatile__ (
//         "outb %[data], %[port];"
//         :
//         : [data] "a" (data), [port] "Nd" (port)
//     );
// }
//
// static inline u16 ioport_read16(u16 port)
// {
//     u16 data;
//
//     __asm__ __volatile__ (
//         "inw %[port], %[data];"
//         : [data] "=a" (data)
//         : [port] "Nd" (port)
//     );
//
//     return data;
// }
//
// static inline void ioport_write16(u16 port, u16 data)
// {
//     __asm__ __volatile__ (
//         "outw %[data], %[port];"
//         :
//         : [data] "a" (data), [port] "Nd" (port)
//     );
// }
//
// static inline u32 ioport_read32(u16 port)
// {
//     u16 data;
//
//     __asm__ __volatile__ (
//         "inl %[port], %[data];"
//         : [data] "=a" (data)
//         : [port] "Nd" (port)
//     );
//
//     return data;
// }
//
// static inline void ioport_write32(u16 port, u32 data)
// {
//     __asm__ __volatile__ (
//         "outl %[data], %[port];"
//         :
//         : [data] "a" (data), [port] "Nd" (port)
//     );
// }
//
// static inline u64 ioport_read64(u16 port)
//     { return ioport_read32(port); }
// static inline void ioport_write64(u16 port, u64 data)
//     { ioport_write32(port, data); }
//
//
// #endif
