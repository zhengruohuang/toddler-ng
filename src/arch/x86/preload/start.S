#include "header.h"

#define MULTIBOOT_HEADER_FLAGS (MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_VIDEO_MODE)

.section ".text.boot"

.global _start

.extern preload_entry

_start:
    jmp __real_start

    // Multiboot header
    .align  4
multiboot_header:
    .long   MULTIBOOT_HEADER_MAGIC
    .long   MULTIBOOT_HEADER_FLAGS
    .long   -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
    // No additional fields needed for ELF

__real_start:
    // Set up stack
    movl    $(_start - 0x10), %esp

    // Set up arguments from multiboot
    // so that they can be accessed through both reg and stack
    pushl   %ebx    // Pointer to multiboot info
    pushl   %eax    // Magic

    // Go to loader
    call    preload_entry

    // Should never reach here
__stop:
    jmp     __stop
    nop


#ifdef ARCH_AMD64

.global _call_hal64

.align 16
.code64

// Multiboot info pointer -> rsi
// Multiboot magic -> rdi
// loader_entry -> rdx
_call_hal64:
    jmp     *%rdx
    nop

    // Stop
    jmp     .

#endif
