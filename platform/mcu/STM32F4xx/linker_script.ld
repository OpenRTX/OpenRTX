/*
    RAM space is organized in this way: the stack begins at the end of
    the RAM space and grows down towards the base, while the heap begins
    at the end of the bss space (symbol _end) and grows up towards the
    end of the RAM.

    WARNING: with this configuration we have an high risk of collision
    between stack and heap, since none of them is limited. One thing to
    do in the future is to put this limit to the stack.
 */

/* stack top in large ram, at an address which, masked with 0x2FFE0000,
   gives 0x20000000
*/
_stack_top = 0x2001FFFF;

/* temporary set heap end equal to stack top */
_heap_end = _stack_top;


/* identify the Entry Point  */
ENTRY(Reset_Handler)

/* specify the memory areas  */
MEMORY
{
    flash(rx)   : ORIGIN = 0x0800C000, LENGTH = 1M - 48K
    smallram(wx) : ORIGIN = 0x10000200, LENGTH = 64K
    largeram(wx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* now define the output sections  */
SECTIONS
{
    . = 0;
    
    /* .text section: code goes to flash */
    .text :
    {
        /* Startup code must go at address 0 */
        KEEP(*(.isr_vector))
        
        *(.text)
        *(.text.*)
        *(.gnu.linkonce.t.*)
        /* these sections for thumb interwork? */
        *(.glue_7)
        *(.glue_7t)
        /* these sections for C++? */
        *(.gcc_except_table)
        *(.gcc_except_table.*)
        *(.ARM.extab*)
        *(.gnu.linkonce.armextab.*)

        . = ALIGN(4);
        /* .rodata: constant data */
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)

        /* C++ Static constructors/destructors (eabi) */
        . = ALIGN(4);
        KEEP(*(.init))
        
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
        KEEP(*(.fini))

        . = ALIGN(4);
        __fini_array_start = .;
        KEEP (*(.fini_array))
        KEEP (*(SORT(.fini_array.*)))
        __fini_array_end = .;

        /* C++ Static constructors/destructors (elf)  */
        . = ALIGN(4);
        _ctor_start = .;
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))
       _ctor_end = .;

        . = ALIGN(4);
        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*crtend.o(.dtors))
    } > flash

    /* .ARM.exidx is sorted, so has to go in its own output section.  */
    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > flash
    __exidx_end = .;

    /* .data section: global variables go to ram, but also store a copy to
       flash to initialize them */
    .data : ALIGN(8)
    {
        _data = .;
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(8);
        _edata = .;
    } > smallram AT > flash
    _etext = LOADADDR(.data);

    /* .bss section: uninitialized global variables go to ram */
    .bss :
    {
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        . = ALIGN(8);
    } > largeram
    _bss_end = .;

    _end = .;
    PROVIDE(end = .);
}
