/**
 ******************************************************************************
 * @file      gcc_arm.ld
 * @author    ListenAI application team
 * @brief     Linker script for 6001 Device
 *                      8Mbytes     FLASH
 *                      16Kbytes    DTCMRAM
 *                      16Kbytes    ITCMRAM
 *                      320Kbytes   RAM
 *
 *            Set heap size, stack size and stack location according
 *            to application requirements.
 *
 *            Set memory bank area and size if external memory is used
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 ListenAI.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ListenAI under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

SECTIONS
{
  /*
   * This section stores metadata that may be used during tokenized string
   * decoding. This metadata describes properties that may affect how the
   * tokenized string is encoded or decoded -- the maximum length of the hash
   * function and the sizes of certain integer types.
   *
   * Metadata is declared as key-value pairs. See the metadata variable in
   * tokenize.cc for further details.
   */
  .pw_tokenizer.info 0x0 (INFO) :
  {
    KEEP(*(.pw_tokenizer.info))
  }

  /*
   * Tokenized string entries are stored in this section. Each entry contains
   * the original string literal and the calculated token that represents it. In
   * the compiled code, the token and a compact argument list encoded in a
   * uint32_t are used in place of the format string. The compiled code
   * contains no references to the tokenized string entries in this section.
   *
   * The tokenized string entry format is specified by the
   * pw::tokenizer::internal::Entry class in
   * pw_tokenizer/public/pw_tokenizer/internal/tokenize_string.h.
   *
   * The section contents are declared with KEEP so that they are not removed
   * from the ELF. These are never emitted in the final binary or loaded into
   * memory.
   */
  .pw_tokenizer.entries 0x0 (INFO) :
  {
    KEEP(*(.pw_tokenizer.entries.*))
  }
}

/*---------------------- Flash Configuration ----------------------------------
  <h> Flash Configuration
    <o0> Flash Base Address <0x18000000>
    <o1> Flash Size (in Bytes) <0x00040000>
  </h>
  -----------------------------------------------------------------------------*/
__ROM_BASE = 0x00080000;
__ROM_SIZE = 0x00020000; /* 128KB */
/*__ROM_BASE = 0x18000000;*/
/*__ROM_SIZE = 0x00040000;*/    /* 8MB */

/*--------------------- Embedded RAM Configuration ----------------------------
  <h> RAM Configuration
    <o0> RAM Base Address   default <0x00080000> can be configured to <0x20080000>
    <o1> RAM Size (in Bytes) <0x00050000>
  </h>
 -----------------------------------------------------------------------------*/
__RAM_BASE = 0x000A0000;    /* CODE_RAM + SYS_RAM */
__RAM_SIZE = 0x00020000; /* 192KB - 64KB */
 
/*__RAM_BASE = 0x20080000;*/    /* CODE_RAM + SYS_RAM */
/*__RAM_SIZE = 0x00050000;*/   /* 320KB */

__ITCM_BASE = 0x00010000;   /* ITCM */
__ITCM_SIZE = 0x4000;       /* 16KB */

__DTCM_BASE = 0x20000000;   /* DTCM */
__DTCM_SIZE = 0x4000;       /* 16KB */

/*--------------------- Stack / Heap Configuration ----------------------------
  <h> Stack / Heap Configuration
    <o0> Stack Size (in Bytes) <0x00001000>
    <o1> Heap Size (in Bytes) <0x00001000>
  </h>
  -----------------------------------------------------------------------------*/
__STACK_SIZE = 0x00001000;
__HEAP_SIZE  = 0x00001000;



MEMORY
{
  FLASH (rx)  : ORIGIN = __ROM_BASE, LENGTH = __ROM_SIZE
  ITCM (rwx) : ORIGIN = __ITCM_BASE, LENGTH = __ITCM_SIZE
  DTCM (rwx) : ORIGIN = __DTCM_BASE, LENGTH = __DTCM_SIZE
  RAM  (rwx) : ORIGIN = __RAM_BASE, LENGTH = __RAM_SIZE
}

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __etext2
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 */
ENTRY(Reset_Handler)

SECTIONS
{
  /* The startup code into "FLASH" Rom type memory */
  .text :
  {
  	_stext = .;
    __interrupts_start__ = .;
    . = ALIGN(4);
    KEEP(*(.vectors)) /* Startup code */  
    __interrupts_end__ = .;
    *(.text*)
    KEEP(*(.init))
    KEEP(*(.fini))

    /* .ctors */
    *crtbegin.o(.ctors)
    *crtbegin?.o(.ctors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
    *(SORT(.ctors.*))
    *(.ctors)

    /* .dtors */
    *crtbegin.o(.dtors)
    *crtbegin?.o(.dtors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
    *(SORT(.dtors.*))
    *(.dtors)

    *(.rodata*)
    
    . = ALIGN(4);
    _shell_command_start = .;
	KEEP (*(shellCommand))
	_shell_command_end = .;
    

    KEEP(*(.eh_frame*))
    _etext = .;
  } > FLASH

  /*
   * SG(TrustZone Secure Gateway) veneers:
   * All SG veneers are placed in the special output section .gnu.sgstubs. Its start address
   * must be set, either with the command line option --section-start or in a linker script,
   * to indicate where to place these veneers in memory.
   */
/*
  .gnu.sgstubs :
  {
    . = ALIGN(32);
  } > FLASH
*/
  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > FLASH

  __exidx_start = .;
  .ARM.exidx :
  {
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  } > FLASH
  __exidx_end = .;

  .copy.table :
  {
    . = ALIGN(4);
    __copy_table_start__ = .;
    
    LONG (__etext)
    LONG (__data_start__)
    LONG (__data_end__ - __data_start__)
    
    /* Add each additional data section here */

    LONG (__etext2)
    LONG (__data2_start__)
    LONG (__data2_end__ - __data2_start__)

    LONG (__etext3)
    LONG (__itcm_start__)
    LONG (__itcm_end__ - __itcm_start__)    
    
    __copy_table_end__ = .;
  } > FLASH

  .zero.table :
  {
    . = ALIGN(4);
    __zero_table_start__ = .;
    
    /* Add each additional bss section here */
    LONG (__bss_start__)
    LONG (__bss_end__ - __bss_start__)

    LONG (__bss2_start__)
    LONG (__bss2_end__ - __bss2_start__)

    __zero_table_end__ = .;
  } > FLASH
  
  /**
   * Location counter can end up 2byte aligned with narrow Thumb code but
   * __etext is assumed by startup code to be the LMA of a section in RAM
   * which must be 4byte aligned 
   */
  __etext = ALIGN (4);
  
  .data : AT (__etext)
  {
    __data_start__ = .; 
    *(vtable)
    *(.data)
    *(.data.*)

    . = ALIGN(4);
    __usb_descriptor_start = .;
    *(.usb.des* ) 
    __usb_descriptor_end = .;   
        
    __usb_data_start = .;
    *(.usb.data ) 
    __usb_data_end = .;   
        
    /* preinit data */
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP(*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);

    . = ALIGN(4);
    /* init data */
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    PROVIDE_HIDDEN (__init_array_end = .);


    . = ALIGN(4);
    /* finit data */
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array))
    PROVIDE_HIDDEN (__fini_array_end = .);

    KEEP(*(.jcr*))
    . = ALIGN(4);
    
    /* All data end */
    __data_end__ = .;

  } > RAM


  .bss :
  {
    . = ALIGN(4);
    __bss_start__ = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end__ = .;
  } > RAM AT > RAM

  .heap (COPY) :
  {
    . = ALIGN(8);
    __end__ = .;
    PROVIDE(end = .);
    . = . + __HEAP_SIZE;
    . = ALIGN(8);
    __HeapLimit = .;
  } > RAM

  .stack (ORIGIN(RAM) + LENGTH(RAM) - __STACK_SIZE) (COPY) :
  {
    . = ALIGN(8);
    _sstack = .;
    __StackLimit = .;
    . = . + __STACK_SIZE;
    . = ALIGN(8);
    _estack = .;
    __StackTop = .;
  } > RAM
  PROVIDE(__stack = __StackTop);


  /*
   * Secondary data section, optional
   *
   * Remember to add each additional data section
   * to the .copy.table above to asure proper
   * initialization during startup.
   */

  __etext2 = __etext + ALIGN (SIZEOF(.data), 4);

  .data2 : AT (__etext2)
  {
    . = ALIGN(4);
    __data2_start__ = .;
    *(.segger_rtt )    
    *(.data2)
    *(.data2.*)
    . = ALIGN(4);
    __data2_end__ = .;

  } > DTCM


  /*
   * Secondary bss section, optional
   *
   * Remember to add each additional bss section
   * to the .zero.table above to asure proper
   * initialization during startup.
   */
   
   .bss2(NOLOAD):
  {
    . = ALIGN(4);
    __bss2_start__ = .;
    *(.bss2)
    *(.bss2.*)
    . = ALIGN(4);
    __bss2_end__ = .;
  } > DTCM  
 

/*
  .heap (COPY) :
  {
    . = ALIGN(8);
    __end__ = .;
    PROVIDE(end = .);
    . = . + __HEAP_SIZE;
    . = ALIGN(8);
    __HeapLimit = .;
  } > DTCM


  .stack (ORIGIN(DTCM) + LENGTH(DTCM) - __STACK_SIZE) (COPY) :
  {
    . = ALIGN(8);
    __StackLimit = .;
    . = . + __STACK_SIZE;
    . = ALIGN(8);
    __StackTop = .;
  } > DTCM
  PROVIDE(__stack = __StackTop);
  */
  
  /*size need to be fixed*/
  M_VECTOR_ITCM_SIZE = DEFINED(__flash_vector_table__) ? 0x0 : (__interrupts_end__ - __interrupts_start__);
  
  .interrupts_itcm(NOLOAD):
  {
    . = ALIGN(4);
    __VECTOR_ITCM__ = .;
    __ITCM_START = .;
    __interrupts_itcm_start__ = .; /* Create a global symbol at data start. */
    . += M_VECTOR_ITCM_SIZE;
    . = ALIGN(4);
    __interrupts_itcm_end__ = .;   /* Define a global symbol at data end. */
  } > ITCM

  __VECTOR_ITCM = DEFINED(__flash_vector_table__) ? ORIGIN(FLASH) : __VECTOR_ITCM__;
  __RAM_VECTOR_TABLE_SIZE = DEFINED(__flash_vector_table__) ? 0x0 : (__interrupts_itcm_end__ - __interrupts_itcm_start__);  
  

  __etext3 = __etext + ALIGN (SIZEOF(.data), 4) + ALIGN (SIZEOF(.data2), 4) + ALIGN (SIZEOF(.bss2), 4);
  
  .itcm : AT (__etext3)
  {
    . = ALIGN(4);
    __itcm_start__ = .;  
    *(.itcm ) 
    . = ALIGN(4);
    __itcm_end__ = .;       
  } > ITCM   
  
  
  /* Check if data + heap + stack exceeds RAM limit */
  ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
}
