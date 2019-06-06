#ifndef __ARCH_X86_LOADER_INCLUDE_HEADER_H__
#define __ARCH_X86_LOADER_INCLUDE_HEADER_H__


/*
 * Multiboot header
 */
// Magics
#define MULTIBOOT_HEADER_MAGIC      0x1BADB002  // Multiboot header magic
#define MULTIBOOT_BOOTLOADER_MAGIC  0x2BADB002  // Should be in %eax

// Flags
#define MULTIBOOT_PAGE_ALIGN        0x00000001  // Align all boot modules on i386 page (4KB) boundaries
#define MULTIBOOT_MEMORY_INFO       0x00000002  // Must pass memory information to OS
#define MULTIBOOT_VIDEO_MODE        0x00000004  // Must pass video information to OS
#define MULTIBOOT_AOUT_KLUDGE       0x00010000  // Use address fields in the header


#endif
