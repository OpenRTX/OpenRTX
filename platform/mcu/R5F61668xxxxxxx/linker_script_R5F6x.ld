/* Default linker script, for normal executables */
OUTPUT_FORMAT("srec")
OUTPUT_ARCH(h8300sx)
ENTRY("_ResetHandler")

MEMORY
{
    /* H8/1668R has 1024 Kbytes of ROM */
    rom      (rx)  : ORIGIN = 0x0000, LENGTH = 1024K

    /* H8/1668R has 56 Kbytes of RAM */
    ram      (rwx) : ORIGIN = 0xFFE00, LENGTH = 56K
}

STACK_SIZE = 0x200;

SECTIONS
{
    .text :
	{
        KEEP(*(.vectors .vectors.*))
        *(.text*)
        *(.rodata)
        *(.strings)
        _etext = . ;
	} > rom

    .data :
	{
        _data = . ;
        *(.data)
        _edata = . ;
	} > ram AT >rom

    .bss (NOLOAD):
	{
        _bss = . ;
        *(.bss)
        *(COMMON)
        _ebss = . ;
	} >ram

    .stack (NOLOAD):
    {
        _stack = . ;
        . = ALIGN(8);
        . = . + STACK_SIZE;
        . = ALIGN(8);
    } > ram
}
