/*
*********************************************************************************************************
*                                               uC/CPU
*                                    CPU CONFIGURATION & PORT LAYER
*
*                    Copyright 2004-2020 Silicon Laboratories Inc. www.silabs.com
*
*                                 SPDX-License-Identifier: APACHE-2.0
*
*               This software is subject to an open source license and is distributed by
*                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
*                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            CPU PORT FILE
*
*                                               ARMv6-M
*
* Filename : cpu_c.c
* Version  : v1.32.00
*********************************************************************************************************
* Note(s)  : This port supports the ARM Cortex-M0, and Cortex-M0+ architectures.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#include  <cpu.h>
#include  <cpu_core.h>

#include  <lib_def.h>

#ifdef __cplusplus
extern  "C" {
#endif


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  CPU_INT_SRC_POS_MAX                  ((((CPU_REG_ICTR + 1) & 0x1F) * 32) + 16)

#define  CPU_BIT_BAND_SRAM_REG_LO                 0x20000000
#define  CPU_BIT_BAND_SRAM_REG_HI                 0x200FFFFF
#define  CPU_BIT_BAND_SRAM_BASE                   0x22000000


#define  CPU_BIT_BAND_PERIPH_REG_LO               0x40000000
#define  CPU_BIT_BAND_PERIPH_REG_HI               0x400FFFFF
#define  CPU_BIT_BAND_PERIPH_BASE                 0x42000000


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*
* Note(s) : (1) Determines the interrupt programmable priority levels. This is normally specified in the
*               Microcontroller reference manual. 2-bits gives us 4 programmable priority levels.
*
*                     NVIC_IPRx
*                 7                0
*                +------------------+
*                |       PRIO       |
*                +------------------+
*
*                Bits[7:6] Priority mask bits
*                Bits[5:0] Reserved
*
*                In this example our CPU_CFG_NVIC_PRIO_BITS define should be set to 2 due to the processor
*                implementing only bits[7:6].
*********************************************************************************************************
*/

#ifndef CPU_CFG_NVIC_PRIO_BITS
#error  "CPU_CFG_NVIC_PRIO_BITS         not #define'd in 'cpu_cfg.h'    "   /* See Note # 1            */
#endif


/*
*********************************************************************************************************
*                                          CPU_BitBandClr()
*
* Description : Clear bit in bit-band region.
*
* Argument(s) : addr            Byte address in memory space.
*
*               bit_nbr         Bit number in byte.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  CPU_BitBandClr (CPU_ADDR    addr,
                      CPU_INT08U  bit_nbr)
{
    CPU_ADDR  bit_word_off;
    CPU_ADDR  bit_word_addr;


    if ((addr >= CPU_BIT_BAND_SRAM_REG_LO) &&
        (addr <= CPU_BIT_BAND_SRAM_REG_HI)) {
        bit_word_off  = ((addr - CPU_BIT_BAND_SRAM_REG_LO  ) * 32) + (bit_nbr * 4);
        bit_word_addr = CPU_BIT_BAND_SRAM_BASE   + bit_word_off;

      *(volatile CPU_INT32U *)(bit_word_addr) = 0;

    } else if ((addr >= CPU_BIT_BAND_PERIPH_REG_LO) &&
               (addr <= CPU_BIT_BAND_PERIPH_REG_HI)) {
        bit_word_off  = ((addr - CPU_BIT_BAND_PERIPH_REG_LO) * 32) + (bit_nbr * 4);
        bit_word_addr = CPU_BIT_BAND_PERIPH_BASE + bit_word_off;

      *(volatile CPU_INT32U *)(bit_word_addr) = 0;
    }
}


/*
*********************************************************************************************************
*                                          CPU_BitBandSet()
*
* Description : Set bit in bit-band region.
*
* Argument(s) : addr            Byte address in memory space.
*
*               bit_nbr         Bit number in byte.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  CPU_BitBandSet (CPU_ADDR    addr,
                      CPU_INT08U  bit_nbr)
{
    CPU_ADDR  bit_word_off;
    CPU_ADDR  bit_word_addr;


    if ((addr >= CPU_BIT_BAND_SRAM_REG_LO) &&
        (addr <= CPU_BIT_BAND_SRAM_REG_HI)) {
        bit_word_off  = ((addr - CPU_BIT_BAND_SRAM_REG_LO  ) * 32) + (bit_nbr * 4);
        bit_word_addr = CPU_BIT_BAND_SRAM_BASE   + bit_word_off;

      *(volatile CPU_INT32U *)(bit_word_addr) = 1;

    } else if ((addr >= CPU_BIT_BAND_PERIPH_REG_LO) &&
               (addr <= CPU_BIT_BAND_PERIPH_REG_HI)) {
        bit_word_off  = ((addr - CPU_BIT_BAND_PERIPH_REG_LO) * 32) + (bit_nbr * 4);
        bit_word_addr = CPU_BIT_BAND_PERIPH_BASE + bit_word_off;

      *(volatile CPU_INT32U *)(bit_word_addr) = 1;
    }
}


/*
*********************************************************************************************************
*                                           CPU_IntSrcDis()
*
* Description : Disable an interrupt source.
*
* Argument(s) : pos     Position of interrupt vector in interrupt table :
*
*                           0       Invalid (see Note #1a).
*                           1       Invalid (see Note #1b).
*                           2       Non-maskable Interrupt.
*                           3       Hard Fault.
*                           4       Memory Management.
*                           5       Bus Fault.
*                           6       Usage Fault.
*                           7-10    Reserved.
*                           11      SVCall.
*                           12      Debug Monitor.
*                           13      Reserved.
*                           14      PendSV.
*                           15      SysTick.
*                           16+     External Interrupt.
*
* Return(s)   : none.
*
* Note(s)     : (1) Several table positions do not contain interrupt sources :
*
*                   (a) Position 0 contains the stack pointer.
*                   (b) Positions 7-10, 13 are reserved.
*
*               (2) Several interrupts cannot be disabled/enabled :
*
*                   (a) Reset.
*                   (b) NMI.
*                   (c) Hard fault.
*                   (d) SVCall.
*                   (e) Debug monitor.
*                   (f) PendSV.
*
*               (3) The maximum Cortex-M0 table position is 256.  A particular Cortex-M0 may have fewer
*                   than 240 external exceptions and, consequently, fewer than 256 table positions.
*                   This function assumes that the specified table position is valid if the interrupt
*                   controller type register's INTLINESNUM field is large enough so that the position
*                   COULD be valid.
*********************************************************************************************************
*/

void  CPU_IntSrcDis (CPU_INT08U  pos)
{
    CPU_INT08U  group;
    CPU_INT08U  pos_max;
    CPU_INT08U  nbr;
    CPU_SR_ALLOC();


    switch (pos) {
        case CPU_INT_STK_PTR:                                   /* ---------------- INVALID OR RESERVED --------------- */
        case CPU_INT_RSVD_07:
        case CPU_INT_RSVD_08:
        case CPU_INT_RSVD_09:
        case CPU_INT_RSVD_10:
        case CPU_INT_RSVD_13:
             break;


                                                                /* ----------------- SYSTEM EXCEPTIONS ---------------- */
        case CPU_INT_RESET:                                     /* Reset (see Note #2).                                 */
        case CPU_INT_NMI:                                       /* Non-maskable interrupt (see Note #2).                */
        case CPU_INT_HFAULT:                                    /* Hard fault (see Note #2).                            */
        case CPU_INT_SVCALL:                                    /* SVCall (see Note #2).                                */
        case CPU_INT_PENDSV:                                    /* PendSV (see Note #2).                                */
             break;

        case CPU_INT_SYSTICK:                                   /* SysTick.                                             */
             CPU_CRITICAL_ENTER();
             CPU_REG_SYST_CSR  &= ~CPU_REG_SYST_CSR_ENABLE;
             CPU_CRITICAL_EXIT();
             break;


                                                                /* ---------------- EXTERNAL INTERRUPT ---------------- */
        default:
             pos_max = CPU_INT_SRC_POS_MAX;
             if (pos < pos_max) {                               /* See Note #3.                                         */
                 group = (pos - 16) / 32;
                 nbr   = (pos - 16) % 32;

                 CPU_CRITICAL_ENTER();
                 CPU_REG_NVIC_ICER(group) = DEF_BIT(nbr);       /* Disable interrupt.                                   */
                 CPU_CRITICAL_EXIT();
             }
             break;
    }
}


/*
*********************************************************************************************************
*                                           CPU_IntSrcEn()
*
* Description : Enable an interrupt source.
*
* Argument(s) : pos     Position of interrupt vector in interrupt table (see 'CPU_IntSrcDis()').
*
* Return(s)   : none.
*
* Note(s)     : (1) See 'CPU_IntSrcDis()  Note #1'.
*
*               (2) See 'CPU_IntSrcDis()  Note #2'.
*
*               (3) See 'CPU_IntSrcDis()  Note #3'.
*********************************************************************************************************
*/

void  CPU_IntSrcEn (CPU_INT08U  pos)
{
    CPU_INT08U  group;
    CPU_INT08U  nbr;
    CPU_INT08U  pos_max;
    CPU_SR_ALLOC();


    switch (pos) {
        case CPU_INT_STK_PTR:                                   /* ---------------- INVALID OR RESERVED --------------- */
        case CPU_INT_RSVD_07:
        case CPU_INT_RSVD_08:
        case CPU_INT_RSVD_09:
        case CPU_INT_RSVD_10:
        case CPU_INT_RSVD_13:
             break;


                                                                /* ----------------- SYSTEM EXCEPTIONS ---------------- */
        case CPU_INT_RESET:                                     /* Reset (see Note #2).                                 */
        case CPU_INT_NMI:                                       /* Non-maskable interrupt (see Note #2).                */
        case CPU_INT_HFAULT:                                    /* Hard fault (see Note #2).                            */
        case CPU_INT_SVCALL:                                    /* SVCall (see Note #2).                                */
        case CPU_INT_PENDSV:                                    /* PendSV (see Note #2).                                */
             break;

        case CPU_INT_SYSTICK:                                   /* SysTick.                                             */
             CPU_CRITICAL_ENTER();
             CPU_REG_SYST_CSR  |= CPU_REG_SYST_CSR_ENABLE;
             CPU_CRITICAL_EXIT();
             break;


                                                                /* ---------------- EXTERNAL INTERRUPT ---------------- */
        default:
             pos_max = CPU_INT_SRC_POS_MAX;
             if (pos < pos_max) {                               /* See Note #3.                                         */
                 group = (pos - 16) / 32;
                 nbr   = (pos - 16) % 32;

                 CPU_CRITICAL_ENTER();
                 CPU_REG_NVIC_ISER(group) = DEF_BIT(nbr);       /* Enable interrupt.                                    */
                 CPU_CRITICAL_EXIT();
             }
             break;
    }
}


/*
*********************************************************************************************************
*                                         CPU_IntSrcPendClr()
*
* Description : Clear a pending interrupt.
*
* Argument(s) : pos     Position of interrupt vector in interrupt table (see 'CPU_IntSrcDis()').
*
* Return(s)   : none.
*
* Note(s)     : (1) See 'CPU_IntSrcDis()  Note #1'.
*
*               (2) The pending status of several interrupts cannot be clear/set :
*
*                   (a) Reset.
*                   (b) NMI.
*                   (c) Hard fault.
*                   (d) Memory Managment.
*                   (e) Bus Fault.
*                   (f) Usage Fault.
*                   (g) SVCall.
*                   (h) Debug monitor.
*                   (i) PendSV.
*                   (j) Systick
*
*               (3) See 'CPU_IntSrcDis()  Note #3'.
*********************************************************************************************************
*/

void  CPU_IntSrcPendClr (CPU_INT08U  pos)

{
    CPU_INT08U  group;
    CPU_INT08U  nbr;
    CPU_INT08U  pos_max;
    CPU_SR_ALLOC();


    switch (pos) {
        case CPU_INT_STK_PTR:                                   /* ---------------- INVALID OR RESERVED --------------- */
        case CPU_INT_RSVD_07:
        case CPU_INT_RSVD_08:
        case CPU_INT_RSVD_09:
        case CPU_INT_RSVD_10:
        case CPU_INT_RSVD_13:
             break;
                                                                /* ----------------- SYSTEM EXCEPTIONS ---------------- */
        case CPU_INT_RESET:                                     /* Reset (see Note #2).                                 */
        case CPU_INT_NMI:                                       /* Non-maskable interrupt (see Note #2).                */
        case CPU_INT_HFAULT:                                    /* Hard fault (see Note #2).                            */
        case CPU_INT_SVCALL:                                    /* SVCall (see Note #2).                                */
        case CPU_INT_PENDSV:                                    /* PendSV (see Note #2).                                */
        case CPU_INT_SYSTICK:                                   /* SysTick.                                             */
             break;
                                                                /* ---------------- EXTERNAL INTERRUPT ---------------- */
        default:
             pos_max = CPU_INT_SRC_POS_MAX;
             if (pos < pos_max) {                               /* See Note #3.                                         */
                 group = (pos - 16) / 32;
                 nbr   = (pos - 16) % 32;

                 CPU_CRITICAL_ENTER();
                 CPU_REG_NVIC_ICPR(group) = DEF_BIT(nbr);       /* Clear Pending interrupt.                             */
                 CPU_CRITICAL_EXIT();
             }
             break;
    }
}


/*
*********************************************************************************************************
*                                         CPU_IntSrcPrioSet()
*
* Description : Set priority of an interrupt source.
*
* Argument(s) : pos     Position of interrupt vector in interrupt table (see 'CPU_IntSrcDis()').
*
*               prio    Priority.  Use a lower priority number for a higher priority.
*
* Return(s)   : none.
*
* Note(s)     : (1) See 'CPU_IntSrcDis()  Note #1'.
*
*               (2) Several interrupts priorities CANNOT be set :
*
*                   (a) Reset (always -3).
*                   (b) NMI (always -2).
*                   (c) Hard fault (always -1).
*
*               (3) See 'CPU_IntSrcDis()  Note #3'.
*********************************************************************************************************
*/

void  CPU_IntSrcPrioSet (CPU_INT08U  pos,
                         CPU_INT08U  prio)
{
    CPU_INT08U  group;
    CPU_INT08U  nbr;
    CPU_INT08U  pos_max;
    CPU_INT32U  temp;
    CPU_INT32U  prio_offset;
    CPU_SR_ALLOC();


    prio_offset = (prio << (DEF_OCTET_NBR_BITS - CPU_CFG_NVIC_PRIO_BITS));
    if (prio >= (1u << CPU_CFG_NVIC_PRIO_BITS)) {               /* Priority should not exceed the max allowed by MCU    */
        CPU_SW_Exception();
    }

    switch (pos) {
        case CPU_INT_STK_PTR:                                   /* ---------------- INVALID OR RESERVED --------------- */
        case CPU_INT_RSVD_07:
        case CPU_INT_RSVD_08:
        case CPU_INT_RSVD_09:
        case CPU_INT_RSVD_10:
        case CPU_INT_RSVD_13:
             break;


                                                                /* ----------------- SYSTEM EXCEPTIONS ---------------- */
        case CPU_INT_RESET:                                     /* Reset (see Note #2).                                 */
        case CPU_INT_NMI:                                       /* Non-maskable interrupt (see Note #2).                */
        case CPU_INT_HFAULT:                                    /* Hard fault (see Note #2).                            */
             break;

        case CPU_INT_SVCALL:                                    /* SVCall.                                              */
             CPU_CRITICAL_ENTER();
             temp                 = CPU_REG_SCB_SHPRI2;
             temp                &= ~((CPU_INT32U)DEF_OCTET_MASK << (3 * DEF_OCTET_NBR_BITS));
             temp                |=  ((CPU_INT32U)prio_offset    << (3 * DEF_OCTET_NBR_BITS));
             CPU_REG_SCB_SHPRI2   = temp;
             CPU_CRITICAL_EXIT();
             break;

        case CPU_INT_PENDSV:                                    /* PendSV.                                              */
             CPU_CRITICAL_ENTER();
             temp                 = CPU_REG_SCB_SHPRI3;
             temp                &= ~((CPU_INT32U)DEF_OCTET_MASK << (2 * DEF_OCTET_NBR_BITS));
             temp                |=  ((CPU_INT32U)prio_offset    << (2 * DEF_OCTET_NBR_BITS));
             CPU_REG_SCB_SHPRI3   = temp;
             CPU_CRITICAL_EXIT();
             break;

        case CPU_INT_SYSTICK:                                   /* SysTick.                                             */
             CPU_CRITICAL_ENTER();
             temp                 = CPU_REG_SCB_SHPRI3;
             temp                &= ~((CPU_INT32U)DEF_OCTET_MASK << (3 * DEF_OCTET_NBR_BITS));
             temp                |=  ((CPU_INT32U)prio_offset    << (3 * DEF_OCTET_NBR_BITS));
             CPU_REG_SCB_SHPRI3   = temp;
             CPU_CRITICAL_EXIT();
             break;


                                                                /* ---------------- EXTERNAL INTERRUPT ---------------- */
        default:
             pos_max = CPU_INT_SRC_POS_MAX;
             if (pos < pos_max) {                               /* See Note #3.                                         */
                 group                    = (pos - 16) / 4;
                 nbr                      = (pos - 16) % 4;

                 CPU_CRITICAL_ENTER();
                 temp                     = CPU_REG_NVIC_IPR(group);
                 temp                    &= ~((CPU_INT32U)DEF_OCTET_MASK << (nbr * DEF_OCTET_NBR_BITS));
                 temp                    |=  ((CPU_INT32U)prio_offset    << (nbr * DEF_OCTET_NBR_BITS));
                 CPU_REG_NVIC_IPR(group)  = temp;               /* Set interrupt priority.                              */
                 CPU_CRITICAL_EXIT();
             }
             break;
    }
}


/*
*********************************************************************************************************
*                                         CPU_IntSrcPrioGet()
*
* Description : Get priority of an interrupt source.
*
* Argument(s) : pos     Position of interrupt vector in interrupt table (see 'CPU_IntSrcDis()').
*
* Return(s)   : Priority of interrupt source.  If the interrupt source specified is invalid, then
*               DEF_INT_16S_MIN_VAL is returned.
*
* Note(s)     : (1) See 'CPU_IntSrcDis()      Note #1'.
*
*               (2) See 'CPU_IntSrcPrioSet()  Note #2'.
*
*               (3) See 'CPU_IntSrcDis()      Note #3'.
*********************************************************************************************************
*/

CPU_INT16S  CPU_IntSrcPrioGet (CPU_INT08U  pos)
{
    CPU_INT08U  group;
    CPU_INT08U  nbr;
    CPU_INT08U  pos_max;
    CPU_INT16S  prio;
    CPU_INT32U  temp;
    CPU_SR_ALLOC();


    switch (pos) {
        case CPU_INT_STK_PTR:                                   /* ---------------- INVALID OR RESERVED --------------- */
        case CPU_INT_RSVD_07:
        case CPU_INT_RSVD_08:
        case CPU_INT_RSVD_09:
        case CPU_INT_RSVD_10:
        case CPU_INT_RSVD_13:
             prio = DEF_INT_16S_MIN_VAL;
             break;


                                                                /* ----------------- SYSTEM EXCEPTIONS ---------------- */
        case CPU_INT_RESET:                                     /* Reset (see Note #2).                                 */
             prio = -3;
             break;

        case CPU_INT_NMI:                                       /* Non-maskable interrupt (see Note #2).                */
             prio = -2;
             break;

        case CPU_INT_HFAULT:                                    /* Hard fault (see Note #2).                            */
             prio = -1;
             break;

        case CPU_INT_SVCALL:                                    /* SVCall.                                              */
             CPU_CRITICAL_ENTER();
             temp = CPU_REG_SCB_SHPRI2;
             prio = (temp >> (3 * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
             CPU_CRITICAL_EXIT();
             break;

        case CPU_INT_PENDSV:                                    /* PendSV.                                              */
             CPU_CRITICAL_ENTER();
             temp = CPU_REG_SCB_SHPRI3;
             prio = (temp >> (2 * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
             CPU_CRITICAL_EXIT();
             break;

        case CPU_INT_SYSTICK:                                   /* SysTick.                                             */
             CPU_CRITICAL_ENTER();
             temp = CPU_REG_SCB_SHPRI3;
             prio = (temp >> (3 * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
             CPU_CRITICAL_EXIT();
             break;


                                                                /* ---------------- EXTERNAL INTERRUPT ---------------- */
        default:
             pos_max = CPU_INT_SRC_POS_MAX;
             if (pos < pos_max) {                               /* See Note #3.                                         */
                 group = (pos - 16) / 4;
                 nbr   = (pos - 16) % 4;

                 CPU_CRITICAL_ENTER();
                 temp  = CPU_REG_NVIC_IPR(group);               /* Read group interrupt priority.                       */
                 CPU_CRITICAL_EXIT();

                 prio  = (temp >> (nbr * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
             } else {
                 prio  = DEF_INT_16S_MIN_VAL;
             }
             break;
    }

    if (prio != DEF_INT_16S_MIN_VAL) {
        prio = (prio >> (DEF_OCTET_NBR_BITS - CPU_CFG_NVIC_PRIO_BITS));
    }

    return (prio);
}


/*
*********************************************************************************************************
*                                         CPU_RevBits()
*Description : Reverses the bits in a data value.
*
* Prototypes  : CPU_DATA  CPU_RevBits(CPU_DATA  val);
*
* Argument(s) : val         Data value to reverse bits.
*
* Return(s)   : Value with all bits in 'val' reversed (see Note #1).
*
* Note(s)     : (1) val is a 32-bit number
*               (2) Goes through a number and checks for sets bits which are then set
*                   in the reverse locations:
*
*                   reverse_val                 =>     0b00000....00
*                   val                         =>     0101100....10
*                   val's 2nd bit is set        =>     reverse_val's bit (num_bits - 1 - count)
*                   val's 5th bit is set        =>     reverse_val's bit (num_bits - 1 - count)
*                   ...                                ...
*                   ...                                ...
*
*********************************************************************************************************
*/

CPU_DATA CPU_RevBits(CPU_DATA val)
{
    CPU_DATA    reverse_val;
    CPU_INT08U  nbr_bits;                                       /* establish how many bits are in val                   */
    CPU_INT32U  cnt;                                            /* for stepping through each bit in val                 */
    CPU_INT32U  tmp;                                            /* gets shifted off bit to check if set or not          */


    nbr_bits    = sizeof(CPU_DATA) * 8;
    reverse_val = 0;                                            /* make sure reverse_val is cleared out to zeros        */

    for (cnt = 0; cnt < nbr_bits; cnt++)
    {
        tmp = (val & (1 << cnt));                               /* shift the next bit into tmp                          */

        if(tmp) {
            reverse_val |= (1 << ((nbr_bits - 1) - cnt));       /* shift in a 1 bit to reverse equivalent bit           */
        }
    }

    return (reverse_val);
}

#ifdef __cplusplus
}
#endif
