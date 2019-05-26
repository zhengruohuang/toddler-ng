#include "common/include/mach.h"

ENTRY(_start)

SECTIONS
{
/*     . = 0xfffffc0001010000; */
    . = LOADER_BASE_LD;
    __start = .;

    .text :
    {
        __text_start = .;
        *(.text.boot)
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        __text_end = .;
    }

    .data :
    {
        __data_start = .;
        *(.data)
        *(.data*)
        *(.data.stack)
        __data_end = .;
    }

    .bss :
    {
        __bss_start = .;
        *(.bss)
        *(.bss*)
        *(.*bss)
        *(.*bss*)
        *(COMMON)
        __bss_end = .;
    }

    .sdata :
    {
         *(.sdata)
         *(.sdata.*)
    }

    .scommon :
    {
        *(.scommon)
        *(.scommon.*)
    }

    /DISCARD/ :
    {
        *(.pdr)
        *(.comment)
        *(.gnu*)
    }

    __end = .;
}
