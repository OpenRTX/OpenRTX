/*
 * Linker script for Renesas R5F61668 MCU
 */

ENTRY(_Reset_Handler)

STACK_SIZE = 0x200;

MEMORY
{
    flash(rx) : ORIGIN = 0x000000, LENGTH = 1024K    /* 1024 Kbytes of ROM */
    ram(rwx)  : ORIGIN = 0xFEE000, LENGTH = 56K      /* 56 Kbytes of RAM   */
}

SECTIONS
{
    .text :
    {
        KEEP(*(.vectors .vectors.*))
        *(.text*)
        *(.rodata)
        *(.strings)
        _etext = . ;
    } > flash

    .data :
    {
        _data = . ;
        *(.data)
        _edata = . ;
    } > ram AT > flash

    .bss (NOLOAD):
    {
        _bss_start = . ;
        *(.bss)
        *(COMMON)
        _bss_end = . ;
    } > ram

    .stack (NOLOAD):
    {
        _stack = . ;
        . = ALIGN(8);
        . = . + STACK_SIZE;
        . = ALIGN(8);
    } > ram
}
