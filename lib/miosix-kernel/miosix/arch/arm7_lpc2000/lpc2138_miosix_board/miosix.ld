/*****************************************************************************/
/* C++ enabled linker script. V2.00 -- Designed for Miosix Embedded OS       */
/*                                                                           */
/*                           MEMORY MAP                                      */
/*             |                                |                            */
/*   .-------->|--------------------------------|0x40008000                  */
/*   .         |  32Bytes reserved for flash    |0x40007FFF                  */
/*   .         |          programming           |                            */
/*   .-------->|--------------------------------|0x40007FE0 _stack_end       */
/*   .         |    UNDEF Stack                 |                            */
/*   .         |--------------------------------|                            */
/*   .         |    ABORT Stack                 |                            */
/*   .         |--------------------------------|                            */
/*   .         |    FIQ Stack                   |                            */
/*   .         |--------------------------------|                            */
/*   .         |    IRQ Stack                   |                            */
/*   .         |--------------------------------|                            */
/*   ram       |    SVC Stack                   |                            */
/*   .         |--------------------------------| _stack_start, _heap_end    */
/*   .         |                                |                            */
/*   .         |    heap                        |                            */
/*   .         |                                |                            */
/*   .         |--------------------------------| _bss_end, _end             */
/*   .         |  .bss  uninitialized variables |                            */
/*   .         |--------------------------------| _bss_start, _edata         */
/*   .         |  .data initialized variables   |                            */
/*   .-------->|--------------------------------|0x40000000 _data            */
/*             |                                |                            */
/*                             .                                             */
/*                             .                                             */
/*                             .                                             */
/*             |                                |                            */
/*  .--------> |--------------------------------|0x0007D000                  */
/*  .          |                                |                            */
/*  .          |                                |                            */
/*  .          |         unused flash           |                            */
/*  .          |                                |                            */
/*  .          |                                |                            */
/*  .          |--------------------------------|                            */
/*  .          |                                |                            */
/*  .          |      copy of .data area        |                            */
/*  .          |                                |                            */
/*  .          |--------------------------------| _etext                     */
/*  .          |                                |                            */
/*  flash      |                                |                            */
/*  .          |                                |                            */
/*  .          |             code               |                            */
/*  .          |                                |                            */
/*  .          |                                |                            */
/*  .          |                                |                            */
/*  .          |--------------------------------|                            */
/*  .          |         Startup Code           |                            */
/*  .          |         (assembler)            |                            */
/*  .          |--------------------------------|0x00000040 Reset_Handler    */
/*  .          |       interrupt vectors        |                            */
/*  .--------->|--------------------------------|0x00000000 _startup         */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

/* stack sizes, used by startup assembler code, can be modified if needed   */
_und_stack_size = 0x00000000;  /* stack for "UND" is 0, (shared with "FIQ") */
_abt_stack_size = 0x00000000;  /* stack for "ABT" is 0, (shared with "FIQ") */
_fiq_stack_size = 0x00000030;  /* stack for "FIQ"         48 bytes */
_irq_stack_size = 0x00000000;  /* stack for "IRQ" is 0, (shared with "SVC") */
_svc_stack_size = 0x00000200;  /* stack for "SVC"        512 bytes */
_sys_stack_size = 0x00000000;  /* stack for "SYS" is 0, since this stack is only used at startup */

/* check stack sizes */
ASSERT(_und_stack_size % 8 == 0, "UND stack size error");
ASSERT(_abt_stack_size % 8 == 0, "ABT stack size error");
ASSERT(_fiq_stack_size % 8 == 0, "FIQ stack size error");
ASSERT(_irq_stack_size % 8 == 0, "IRQ stack size error");
ASSERT(_svc_stack_size % 8 == 0, "SVC stack size error");
ASSERT(_sys_stack_size % 8 == 0, "SYS stack size error");

/* end of the stack */
_stack_end = 0x40007FE0;       /* end of available ram */

/* calculate the stacks and the end of the heap, used to check heap overflow */
_und_stack_top = _stack_end;
_abt_stack_top = _und_stack_top - _und_stack_size;
_fiq_stack_top = _abt_stack_top - _abt_stack_size;
_irq_stack_top = _fiq_stack_top - _fiq_stack_size;
_svc_stack_top = _irq_stack_top - _irq_stack_size;
_sys_stack_top = _svc_stack_top - _svc_stack_size;
_stack_start   = _sys_stack_top - _sys_stack_size;
_heap_end      = _stack_start;

/* identify the Entry Point  */
ENTRY(_startup)

/* specify the LPC2138 memory areas  */
MEMORY
{
    flash(rx)   : ORIGIN = 0,          LENGTH = 500K  /* 512K-12K bootloader */
    ram(wx)     : ORIGIN = 0x40000000, LENGTH = 32736 /* free RAM area       */
}

/* now define the output sections  */
SECTIONS
{
    . = 0;
    /* Startup code must go a address 0 */
    .startup :
    {
        *stage_1_boot.o (.text)
    } > flash

    /* .text section: code goes to flash */
    .text :
    {
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
        . = ALIGN(0x4);
        _ctor_start = .;
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))
       _ctor_end = .;

        . = ALIGN(0x4);
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
    } > ram AT > flash
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
