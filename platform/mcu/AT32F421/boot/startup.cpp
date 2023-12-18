/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO,                            *
 *                         Federico Terraneo                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

/*
 * startup.cpp
 * STM32 C++ startup.
 * NOTE: for stm32f4 devices ONLY.
 * Supports interrupt handlers in C++ without extern "C"
 * Developed by Terraneo Federico, based on ST startup code.
 * Additionally modified to boot Miosix.
 */

#include "interfaces/arch_registers.h"
#include "kernel/stage_2_boot.h"
#include "core/interrupts.h" //For the unexpected interrupt call
#include <string.h>

/**
 * Called by Reset_Handler, performs initialization and calls main.
 * Never returns.
 */
void program_startup() __attribute__((noreturn));
void program_startup()
{
    //Cortex M3 core appears to get out of reset with interrupts already enabled
    __disable_irq();

    //SystemInit() is called *before* initializing .data and zeroing .bss
    //Despite all startup files provided by ST do the opposite, there are three
    //good reasons to do so:
    //First, the CMSIS specifications say that SystemInit() must not access
    //global variables, so it is actually possible to call it before
    //Second, when running Miosix with the xram linker scripts .data and .bss
    //are placed in the external RAM, so we *must* call SystemInit(), which
    //enables xram, before touching .data and .bss
    //Third, this is a performance improvement since the loops that initialize
    //.data and zeros .bss now run with the CPU at full speed instead of 8MHz
    SystemInit();

    //These are defined in the linker script
    extern unsigned char _etext asm("_etext");
    extern unsigned char _data asm("_data");
    extern unsigned char _edata asm("_edata");
    extern unsigned char _bss_start asm("_bss_start");
    extern unsigned char _bss_end asm("_bss_end");

    //Initialize .data section, clear .bss section
    unsigned char *etext=&_etext;
    unsigned char *data=&_data;
    unsigned char *edata=&_edata;
    unsigned char *bss_start=&_bss_start;
    unsigned char *bss_end=&_bss_end;
    memcpy(data, etext, edata-data);
    memset(bss_start, 0, bss_end-bss_start);

    //Move on to stage 2
    _init();

    //If main returns, reboot
    NVIC_SystemReset();
    for(;;) ;
}

/**
 * Reset handler, called by hardware immediately after reset
 */
void Reset_Handler() __attribute__((__interrupt__, noreturn));
void Reset_Handler()
{
    /*
     * Initialize process stack and switch to it.
     * This is required for booting Miosix, a small portion of the top of the
     * heap area will be used as stack until the first thread starts. After,
     * this stack will be abandoned and the process stack will point to the
     * current thread's stack.
     */
    asm volatile("ldr r0,  =_heap_end          \n\t"
                 "msr psp, r0                  \n\t"
                 "movw r0, #2                  \n\n" //Privileged, process stack
                 "msr control, r0              \n\t"
                 "isb                          \n\t":::"r0");

    program_startup();
}

/**
 * All unused interrupts call this function.
 */
extern "C" void Default_Handler()
{
    unexpectedInterrupt();
}

//System handlers
void /*__attribute__((weak))*/ Reset_Handler();     //These interrupts are not
void /*__attribute__((weak))*/ NMI_Handler();       //weak because they are
void /*__attribute__((weak))*/ HardFault_Handler(); //surely defined by Miosix
void /*__attribute__((weak))*/ MemManage_Handler();
void /*__attribute__((weak))*/ BusFault_Handler();
void /*__attribute__((weak))*/ UsageFault_Handler();
void /*__attribute__((weak))*/ SVC_Handler();
void /*__attribute__((weak))*/ DebugMon_Handler();
void /*__attribute__((weak))*/ PendSV_Handler();
void /*__attribute__((weak))*/ SysTick_Handler();

//Interrupt handlers
void __attribute__((weak)) WWDT_IRQHandler();
void __attribute__((weak)) PVM_IRQHandler();
void __attribute__((weak)) ERTC_IRQHandler();
void __attribute__((weak)) FLASH_IRQHandler();
void __attribute__((weak)) CRM_IRQHandler();
void __attribute__((weak)) EXINT1_0_IRQHandler();
void __attribute__((weak)) EXINT3_2_IRQHandler();
void __attribute__((weak)) EXINT15_4_IRQHandler();
void __attribute__((weak)) DMA1_Channel1_IRQHandler();
void __attribute__((weak)) DMA1_Channel3_2_IRQHandler();
void __attribute__((weak)) DMA1_Channel5_4_IRQHandler();
void __attribute__((weak)) ADC1_CMP_IRQHandler();
void __attribute__((weak)) TMR1_BRK_OVF_TRG_HALL_IRQHandler();
void __attribute__((weak)) TMR1_CH_IRQHandler();
void __attribute__((weak)) TMR3_GLOBAL_IRQHandler();
void __attribute__((weak)) TMR6_GLOBAL_IRQHandler();
void __attribute__((weak)) TMR14_GLOBAL_IRQHandler();
void __attribute__((weak)) TMR15_GLOBAL_IRQHandler();
void __attribute__((weak)) TMR16_GLOBAL_IRQHandler();
void __attribute__((weak)) TMR17_GLOBAL_IRQHandler();
void __attribute__((weak)) I2C1_EVT_IRQHandler();
void __attribute__((weak)) I2C2_EVT_IRQHandler();
void __attribute__((weak)) SPI1_IRQHandler();
void __attribute__((weak)) SPI2_IRQHandler();
void __attribute__((weak)) USART1_IRQHandler();
void __attribute__((weak)) USART2_IRQHandler();
void __attribute__((weak)) I2C1_ERR_IRQHandler();
void __attribute__((weak)) I2C2_ERR_IRQHandler();

//Stack top, defined in the linker script
extern char _main_stack_top asm("_main_stack_top");

//Interrupt vectors, must be placed @ address 0x00000000
//The extern declaration is required otherwise g++ optimizes it out
extern void (* const __Vectors[])();
void (* const __Vectors[])() __attribute__ ((section(".isr_vector"))) =
{
    reinterpret_cast<void (*)()>(&_main_stack_top),/* Stack pointer*/
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0,
    0,
    0,
    0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,

    WWDT_IRQHandler,                     /* Window Watchdog Timer                   */
    PVM_IRQHandler,                      /* PVM through EXINT Line detect           */
    ERTC_IRQHandler,                     /* ERTC                                    */
    FLASH_IRQHandler,                    /* Flash                                   */
    CRM_IRQHandler,                      /* CRM                                     */
    EXINT1_0_IRQHandler,                 /* EXINT Line 1 & 0                        */
    EXINT3_2_IRQHandler,                 /* EXINT Line 3 & 2                        */
    EXINT15_4_IRQHandler,                /* EXINT Line 15 ~ 4                       */
    0,                                   /* Reserved                                */
    DMA1_Channel1_IRQHandler,            /* DMA1 Channel 1                          */
    DMA1_Channel3_2_IRQHandler,          /* DMA1 Channel 3 & 2                      */
    DMA1_Channel5_4_IRQHandler,          /* DMA1 Channel 5 & 4                      */
    ADC1_CMP_IRQHandler,                 /* ADC1 & Comparator                       */
    TMR1_BRK_OVF_TRG_HALL_IRQHandler,    /* TMR1 brake overflow trigger and hall    */
    TMR1_CH_IRQHandler,                  /* TMR1 channel                            */
    0,                                   /* Reserved                                */
    TMR3_GLOBAL_IRQHandler,              /* TMR3                                    */
    TMR6_GLOBAL_IRQHandler,              /* TMR6                                    */
    0,                                   /* Reserved                                */
    TMR14_GLOBAL_IRQHandler,             /* TMR14                                   */
    TMR15_GLOBAL_IRQHandler,             /* TMR15                                   */
    TMR16_GLOBAL_IRQHandler,             /* TMR16                                   */
    TMR17_GLOBAL_IRQHandler,             /* TMR17                                   */
    I2C1_EVT_IRQHandler,                 /* I2C1 Event                              */
    I2C2_EVT_IRQHandler,                 /* I2C2 Event                              */
    SPI1_IRQHandler,                     /* SPI1                                    */
    SPI2_IRQHandler,                     /* SPI2                                    */
    USART1_IRQHandler,                   /* USART1                                  */
    USART2_IRQHandler,                   /* USART2                                  */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    I2C1_ERR_IRQHandler,                 /* I2C1 Error                              */
    0,                                   /* Reserved                                */
    I2C2_ERR_IRQHandler                  /* I2C2 Error                              */
};

#pragma weak WWDT_IRQHandler = Default_Handler
#pragma weak PVM_IRQHandler = Default_Handler
#pragma weak ERTC_IRQHandler = Default_Handler
#pragma weak FLASH_IRQHandler = Default_Handler
#pragma weak CRM_IRQHandler = Default_Handler
#pragma weak EXINT1_0_IRQHandler = Default_Handler
#pragma weak EXINT3_2_IRQHandler = Default_Handler
#pragma weak EXINT15_4_IRQHandler = Default_Handler
#pragma weak DMA1_Channel1_IRQHandler = Default_Handler
#pragma weak DMA1_Channel3_2_IRQHandler = Default_Handler
#pragma weak DMA1_Channel5_4_IRQHandler = Default_Handler
#pragma weak ADC1_CMP_IRQHandler = Default_Handler
#pragma weak TMR1_BRK_OVF_TRG_HALL_IRQHandler = Default_Handler
#pragma weak TMR1_CH_IRQHandler = Default_Handler
#pragma weak TMR3_GLOBAL_IRQHandler = Default_Handler
#pragma weak TMR6_GLOBAL_IRQHandler = Default_Handler
#pragma weak TMR14_GLOBAL_IRQHandler = Default_Handler
#pragma weak TMR15_GLOBAL_IRQHandler = Default_Handler
#pragma weak TMR16_GLOBAL_IRQHandler = Default_Handler
#pragma weak TMR17_GLOBAL_IRQHandler = Default_Handler
#pragma weak I2C1_EVT_IRQHandler = Default_Handler
#pragma weak I2C2_EVT_IRQHandler = Default_Handler
#pragma weak SPI1_IRQHandler = Default_Handler
#pragma weak SPI2_IRQHandler = Default_Handler
#pragma weak USART1_IRQHandler = Default_Handler
#pragma weak USART2_IRQHandler = Default_Handler
#pragma weak I2C1_ERR_IRQHandler = Default_Handler
#pragma weak I2C2_ERR_IRQHandler = Default_Handler
