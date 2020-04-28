#include "common/include/arch.h"

ENTRY(_start)

SECTIONS
{
    . = HAL_BASE;
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

    /DISCARD/ :
    {
        *(.comment)
        *(.gnu*)
        *(.ARM*)
    }

    __end = .;
}
