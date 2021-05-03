ENTRY(_start)

SECTIONS
{
    /* 1MB + 256KB */
    . = 0x140000;
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

    /DISCARD/ :
    {
        *(.pdr)
        *(.comment)
        *(.gnu*)
    }

    __end = .;
}
