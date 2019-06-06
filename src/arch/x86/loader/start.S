#include "loader/include/header.h"

#define MULTIBOOT_HEADER_FLAGS (MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_VIDEO_MODE)

.section ".text.boot"

.global _start
.global _start_ap

.global _call_hal64

.extern loader_entry
.extern loader_entry_ap

_start:
#if (defined(ARCH_AMD64))
    // Set up stack
    movq    $(_start - 0x10), %rsp
#elif (defined(ARCH_IA32))
    // Set up stack
    movl    $(_start - 0x10), %esp

    // Set up arguments from multiboot
    // so that they can be accessed through both reg and stack
    pushl   %ebx    // Pointer to multiboot info
    pushl   %eax    // Magic
#endif

    // Go to loader
    call    loader_entry

    // Should never reach here
__stop:
    jmp     __stop
    nop


_start_ap:
    // Set up stack
    movl    $(_start - 0x10), %esp

    // Go to loader
    call    loader_entry_ap

    // Should never reach here
    jmp     .
    nop
