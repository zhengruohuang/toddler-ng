#include "common/include/abi.h"

ENTRY(_start)

SECTIONS
{
    . = KERNEL_BASE;
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

    .got :
    {
        _gp = (. + 0x8000);

        __got_start = .;
        *(.got.plt)
        *(.got)
        __got_end = .;
    }

    .reginfo :
    {
        *(.reginfo)
    }

    /DISCARD/ :
    {
        *(.pdr)
        *(.comment)
        *(.gnu*)
    }

    __end = .;
}
