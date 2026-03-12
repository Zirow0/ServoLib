/*
 * Linker script для STM32F411CEU6
 *
 * Пам'ять:
 *   FLASH: 512K at 0x08000000
 *   RAM:   128K at 0x20000000
 */

MEMORY
{
    rom (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    ram (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

EXTERN(vector_table)
ENTRY(reset_handler)

SECTIONS
{
    .text : {
        *(.vectors)     /* Таблиця векторів переривань */
        *(.text*)       /* Код програми */
        . = ALIGN(4);
        *(.rodata*)     /* Read-only дані */
        . = ALIGN(4);

        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP (*(.preinit_array))
        __preinit_array_end = .;

        . = ALIGN(4);
        __init_array_start = .;
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        __init_array_end = .;

        . = ALIGN(4);
        __fini_array_start = .;
        KEEP (*(.fini_array))
        KEEP (*(SORT(.fini_array.*)))
        __fini_array_end = .;
    } >rom

    . = ALIGN(4);
    _etext = .;

    .data : {
        _data = .;
        *(.data*)       /* Ініціалізовані дані */
        . = ALIGN(4);
        _edata = .;
    } >ram AT >rom
    _data_loadaddr = LOADADDR(.data);

    .bss : {
        *(.bss*)        /* Неініціалізовані дані */
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } >ram
}

PROVIDE(_stack = ORIGIN(ram) + LENGTH(ram));
