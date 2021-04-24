#include "common/include/abi.h"
#include "common/include/asm.h"

.global _mp_boot_start
.global _mp_boot_end

.text
.code16

/* Code blow will be copied to 0x8000 */
.balign 0x1000
_mp_boot_start:
    cli
    cld
    ljmp    $0, $0x8080

/* 0x8010 */
.balign 0x10
_gdt32:
    .long 0, 0
    .long 0x0000ffff, 0x00cf9a00    /* flat code */
    .long 0x0000ffff, 0x00cf9200    /* flat data */
    .long 0, 0

/* 0x8030 */
_gdtr32:
    .word _gdtr32 - _gdt32 - 1
    .long 0x8010
    .long 0, 0

/* 0x8040 */
.balign 0x40
_gdt64:
    .long 0, 0
    .long 0x0000ffff, 0x00af9a00    /* flat code */
    .long 0x0000ffff, 0x008f9200    /* flat data */
    .long 0, 0

/* 0x8060 */
_gdtr64:
    .word _gdtr64 - _gdt64 - 1
    .long 0x8040
    .long 0, 0

/* 0x8080 */
.balign 0x40
_mp_boot16:
    xorw    %ax, %ax
    movw    %ax, %ds
    movl    $0x8030, %ebx
    lgdtl   (%ebx)
    movl    %cr0, %eax
    orl     $1, %eax
    movl    %eax, %cr0
    ljmp    $8, $0x80a0

.code32

/* 0x80a0 */
.balign 0x20
_mp_boot32:
    /* Set up segment regs */
    movw    $0x10, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %gs
    movw    %ax, %ss

    /* Set up stack */
    movl    $0x8000, %esp
    subl    $16, %esp

#ifdef ARCH_AMD64
    /* Enable PAE */
    movl    %cr4, %eax
    orl     $0x20, %eax
    movl    %eax, %cr4

    /* Enter compatability mode */
    movl    $0xc0000080, %ecx
    rdmsr
    orl     $0x100, %eax
    wrmsr
#endif

    /* Enable page size extension */
    movl    %cr4, %eax
    orl     $0x10, %eax
    movl    %eax, %cr4

    /* Load page table, located at 0x7008 */
    mov    $0x7008, %ebx
    movl    (%ebx), %eax
    movl    %eax, %cr3

    /* Enable paging */
    movl    %cr0, %eax
    orl     $0x80000000, %eax
    movl    %eax, %cr0

#if (defined(ARCH_AMD64))
    /* Load 64-bit GDT */
    movl    $0x8060, %ebx
    lgdtl   (%ebx)

    /* Jump to loader in long mode */
    jmpl    $0x8, $0x8200
#elif (defined(ARCH_IA32))
    /* Loader MP entry addr is stored at 0x7000 */
    movl    $0x7000, %ebx
    movl    (%ebx), %ebx

    /* Jump to loader */
    jmpl    *%ebx
#endif

    /* Should never reach here */
    jmp     .

#if (defined(ARCH_AMD64))
/* 0x8200 */
.code64
.balign 0x200
_mp_boot64:
    /* Loader MP entry addr is stored at 0x7000 */
    movq    $0x7000, %rbx
    movq    (%rbx), %rbx

    jmp     *%rbx
#endif

.balign 0x20
_mp_boot_end:
