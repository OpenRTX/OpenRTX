/*
 * C++ enabled linker script for stm32f769ni (2M FLASH, 512K RAM, 16MB XRAM)
 * Developed by TFT: Terraneo Federico Technologies
 * Optimized for use with the Miosix kernel
 */

/*
 * This linker script puts:
 * - read only data and code (.text, .rodata, .eh_*) in FLASH
 * - the 512Byte main (IRQ) stack, .data and .bss in the DTCM 128KB RAM
 * - .data, .bss, stacks and heap in the external 16MB SDRAM.
 */

/*
 * The main stack is used for interrupt handling by the kernel.
 *
 * *** Readme ***
 * This linker script places the main stack (used by the kernel for interrupts)
 * at the bottom of the ram, instead of the top. This is done for two reasons:
 *
 * - as an optimization for microcontrollers with little ram memory. In fact
 *   the implementation of malloc from newlib requests memory to the OS in 4KB
 *   block (except the first block that can be smaller). This is probably done
 *   for compatibility with OSes with an MMU and paged memory. To see why this
 *   is bad, consider a microcontroller with 8KB of ram: when malloc finishes
 *   up the first 4KB it will call _sbrk_r asking for a 4KB block, but this will
 *   fail because the top part of the ram is used by the main stack. As a
 *   result, the top part of the memory will not be used by malloc, even if
 *   available (and it is nearly *half* the ram on an 8KB mcu). By placing the
 *   main stack at the bottom of the ram, the upper 4KB block will be entirely
 *   free and available as heap space.
 *
 * - In case of main stack overflow the cpu will fault because access to memory
 *   before the beginning of the ram faults. Instead with the default stack
 *   placement the main stack will silently collide with the heap.
 * Note: if increasing the main stack size also increase the ORIGIN value in
 * the MEMORY definitions below accordingly.
 */

_main_stack_size = 512;                             /* main stack = 512Bytes */
_main_stack_top  = 0x20000000 + _main_stack_size;
ASSERT(_main_stack_size   % 8 == 0, "MAIN stack size error");

/* Mapping the heap into XRAM */
_heap_end = 0xc0000000 + 16M;                        /* end of available ram */

/* Identify the Entry Point  */
ENTRY(_Z13Reset_Handlerv)

/*
 * Specify the memory areas
 *
 * NOTE: starting at 0x20000000 there's 128KB of DTCM (Data Tightly Coupled
 * Memory). Technically, we could use this as normal RAM as there's a way for
 * the DMA to access it, but the datasheet is unclear about performance
 * penalties for doing so. To avoid nonuniform DMA memory access latencies,
 * we leave this 128KB DTCM unused except for the first 512Bytes which are for
 * the interrupt stack. This leaves us with 384KB of RAM
 */
MEMORY
{
    xram(wx)  : ORIGIN = 0xc0000000, LENGTH =  16M
    sram(wx)  : ORIGIN = 0x20020000, LENGTH = 384K
    dtcm(wx)  : ORIGIN = 0x20000000, LENGTH = 128K    /* Used for main stack */
    flash(rx) : ORIGIN = 0x08000000, LENGTH =   2M
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
    } > flash

    /* .ARM.exidx is sorted, so has to go in its own output section.  */
    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > flash
    __exidx_end = .;

	/*
     * .data section: global variables go to xram, but also store a copy to
     * flash to initialize them
     */
    .data : ALIGN(8)
    {
        _data = .;
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(8);
        _edata = .;
    } > xram AT > flash
    _etext = LOADADDR(.data);

    /* .bss section: uninitialized global variables go to xram */
    _bss_start = .;
    .bss :
    {
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        . = ALIGN(8);
    } > xram
    _bss_end = .;

    _end = .;
    PROVIDE(end = .);
}
