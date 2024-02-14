/*
 * C++ enabled linker script for stm32 (1M FLASH, 128K RAM) + 2MB xram
 * Developed by TFT: Terraneo Federico Technologies
 * Optimized for use with the Miosix kernel
 */

/*
 * This linker script puts:
 * - all (code, .data, .bss, stacks, heap) in the external ram.
 * It is most useful for debugging, since powercycling the board will erase code
 */

/*
 * The main stack is used for interrupt handling by the kernel.
 * In this linker script the main stack is placed at the top of the ram since:
 * - having 2MB of ram makes the "stack at bottom of ram" optimization useless
 * - the interrupt vectors are forwarded at the bottom of the ram
 */
_main_stack_size   = 0x00000200;  /* main   stack size = 512Bytes */
ASSERT(_main_stack_size   % 8 == 0, "MAIN stack size error");

/* end of the stack */
_main_stack_top = 0x64200000;                     /* placed at the top of ram */
_heap_end = _main_stack_top - _main_stack_size;

/* identify the Entry Point  */
ENTRY(_Z13Reset_Handlerv)

/* specify the memory areas  */
MEMORY
{
    ram(wx)     : ORIGIN = 0x64000000, LENGTH = 2M
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
        __miosix_init_array_start = .;
        KEEP (*(SORT(.miosix_init_array.*)))
        KEEP (*(.miosix_init_array))
        __miosix_init_array_end = .;

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
    } > ram

    /* .ARM.exidx is sorted, so has to go in its own output section.  */
    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > ram
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
    } > ram
    _etext = LOADADDR(.data);

    /* .bss section: uninitialized global variables go to ram */
    _bss_start = .;
    .bss :
    {
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        . = ALIGN(8);
    } > ram
    _bss_end = .;

    _end = .;
    PROVIDE(end = .);
}
