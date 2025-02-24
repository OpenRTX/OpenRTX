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
void __attribute__((weak)) TAMP_STAMP_IRQHandler();
void __attribute__((weak)) ERTC_WKUP_IRQHandler();
void __attribute__((weak)) FLASH_IRQHandler();
void __attribute__((weak)) CRM_IRQHandler();
void __attribute__((weak)) EXINT0_IRQHandler();
void __attribute__((weak)) EXINT1_IRQHandler();
void __attribute__((weak)) EXINT2_IRQHandler();
void __attribute__((weak)) EXINT3_IRQHandler();
void __attribute__((weak)) EXINT4_IRQHandler();
void __attribute__((weak)) DMA1_Channel1_IRQHandler();
void __attribute__((weak)) DMA1_Channel2_IRQHandler();
void __attribute__((weak)) DMA1_Channel3_IRQHandler();
void __attribute__((weak)) DMA1_Channel4_IRQHandler();
void __attribute__((weak)) DMA1_Channel5_IRQHandler();
void __attribute__((weak)) DMA1_Channel6_IRQHandler();
void __attribute__((weak)) DMA1_Channel7_IRQHandler();
void __attribute__((weak)) ADC1_IRQHandler();
void __attribute__((weak)) CAN1_TX_IRQHandler();
void __attribute__((weak)) CAN1_RX0_IRQHandler();
void __attribute__((weak)) CAN1_RX1_IRQHandler();
void __attribute__((weak)) CAN1_SE_IRQHandler();
void __attribute__((weak)) EXINT9_5_IRQHandler();
void __attribute__((weak)) TMR1_BRK_TMR9_IRQHandler();
void __attribute__((weak)) TMR1_OVF_TMR10_IRQHandler();
void __attribute__((weak)) TMR1_TRG_HALL_TMR11_IRQHandler();
void __attribute__((weak)) TMR1_CH_IRQHandler();
void __attribute__((weak)) TMR2_GLOBAL_IRQHandler();
void __attribute__((weak)) TMR3_GLOBAL_IRQHandler();
void __attribute__((weak)) TMR4_GLOBAL_IRQHandler();
void __attribute__((weak)) I2C1_EVT_IRQHandler();
void __attribute__((weak)) I2C1_ERR_IRQHandler();
void __attribute__((weak)) I2C2_EVT_IRQHandler();
void __attribute__((weak)) I2C2_ERR_IRQHandler();
void __attribute__((weak)) SPI1_IRQHandler();
void __attribute__((weak)) SPI2_IRQHandler();
void __attribute__((weak)) USART1_IRQHandler();
void __attribute__((weak)) USART2_IRQHandler();
void __attribute__((weak)) USART3_IRQHandler();
void __attribute__((weak)) EXINT15_10_IRQHandler();
void __attribute__((weak)) ERTCAlarm_IRQHandler();
void __attribute__((weak)) OTGFS1_WKUP_IRQHandler();
void __attribute__((weak)) TMR12_GLOBAL_IRQHandler();
void __attribute__((weak)) TMR13_GLOBAL_IRQHandler();
void __attribute__((weak)) TMR14_GLOBAL_IRQHandler();
void __attribute__((weak)) SPI3_IRQHandler();
void __attribute__((weak)) USART4_IRQHandler();
void __attribute__((weak)) USART5_IRQHandler();
void __attribute__((weak)) TMR6_DAC_GLOBAL_IRQHandler();
void __attribute__((weak)) TMR7_GLOBAL_IRQHandler();
void __attribute__((weak)) DMA2_Channel1_IRQHandler();
void __attribute__((weak)) DMA2_Channel2_IRQHandler();
void __attribute__((weak)) DMA2_Channel3_IRQHandler();
void __attribute__((weak)) DMA2_Channel4_IRQHandler();
void __attribute__((weak)) DMA2_Channel5_IRQHandler();
void __attribute__((weak)) CAN2_TX_IRQHandler();
void __attribute__((weak)) CAN2_RX0_IRQHandler();
void __attribute__((weak)) CAN2_RX1_IRQHandler();
void __attribute__((weak)) CAN2_SE_IRQHandler();
void __attribute__((weak)) OTGFS1_IRQHandler();
void __attribute__((weak)) DMA2_Channel6_IRQHandler();
void __attribute__((weak)) DMA2_Channel7_IRQHandler();
void __attribute__((weak)) USART6_IRQHandler();
void __attribute__((weak)) I2C3_EVT_IRQHandler();
void __attribute__((weak)) I2C3_ERR_IRQHandler();
void __attribute__((weak)) FPU_IRQHandler();
void __attribute__((weak)) USART7_IRQHandler();
void __attribute__((weak)) USART8_IRQHandler();
void __attribute__((weak)) DMAMUX_IRQHandler();
void __attribute__((weak)) ACC_IRQHandler();

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
    TAMP_STAMP_IRQHandler,               /* Tamper and TimeStamps through the EXINT line */
    ERTC_WKUP_IRQHandler,                /* ERTC Wakeup through the EXINT line      */
    FLASH_IRQHandler,                    /* Flash                                   */
    CRM_IRQHandler,                      /* CRM                                     */
    EXINT0_IRQHandler,                   /* EXINT Line 0                            */
    EXINT1_IRQHandler,                   /* EXINT Line 1                            */
    EXINT2_IRQHandler,                   /* EXINT Line 2                            */
    EXINT3_IRQHandler,                   /* EXINT Line 3                            */
    EXINT4_IRQHandler,                   /* EXINT Line 4                            */
    DMA1_Channel1_IRQHandler,            /* DMA1 Channel 1                          */
    DMA1_Channel2_IRQHandler,            /* DMA1 Channel 2                          */
    DMA1_Channel3_IRQHandler,            /* DMA1 Channel 3                          */
    DMA1_Channel4_IRQHandler,            /* DMA1 Channel 4                          */
    DMA1_Channel5_IRQHandler,            /* DMA1 Channel 5                          */
    DMA1_Channel6_IRQHandler,            /* DMA1 Channel 6                          */
    DMA1_Channel7_IRQHandler,            /* DMA1 Channel 7                          */
    ADC1_IRQHandler,                     /* ADC1                                    */
    CAN1_TX_IRQHandler,                  /* CAN1 TX                                 */
    CAN1_RX0_IRQHandler,                 /* CAN1 RX0                                */
    CAN1_RX1_IRQHandler,                 /* CAN1 RX1                                */
    CAN1_SE_IRQHandler ,                 /* CAN1 SE                                 */
    EXINT9_5_IRQHandler ,                /* EXINT Line [9:5]                        */
    TMR1_BRK_TMR9_IRQHandler,            /* TMR1 Brake and TMR9                     */
    TMR1_OVF_TMR10_IRQHandler,           /* TMR1 Overflow and TMR10                 */
    TMR1_TRG_HALL_TMR11_IRQHandler,      /* TMR1 Trigger and hall and TMR11         */
    TMR1_CH_IRQHandler,                  /* TMR1 Channel                            */
    TMR2_GLOBAL_IRQHandler,              /* TMR2                                    */
    TMR3_GLOBAL_IRQHandler,              /* TMR3                                    */
    TMR4_GLOBAL_IRQHandler,              /* TMR4                                    */
    I2C1_EVT_IRQHandler,                 /* I2C1 Event                              */
    I2C1_ERR_IRQHandler,                 /* I2C1 Error                              */
    I2C2_EVT_IRQHandler,                 /* I2C2 Event                              */
    I2C2_ERR_IRQHandler,                 /* I2C2 Error                              */
    SPI1_IRQHandler,                     /* SPI1                                    */
    SPI2_IRQHandler,                     /* SPI2                                    */
    USART1_IRQHandler,                   /* USART1                                  */
    USART2_IRQHandler,                   /* USART2                                  */
    USART3_IRQHandler,                   /* USART3                                  */
    EXINT15_10_IRQHandler,               /* EXINT Line [15:10]                      */
    ERTCAlarm_IRQHandler,                /* RTC Alarm through EXINT Line            */
    OTGFS1_WKUP_IRQHandler,              /* OTGFS1 Wakeup from suspend              */
    TMR12_GLOBAL_IRQHandler,             /* TMR12                                   */
    TMR13_GLOBAL_IRQHandler,             /* TMR13                                   */
    TMR14_GLOBAL_IRQHandler,             /* TMR14                                   */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    SPI3_IRQHandler,                     /* SPI3                                    */
    USART4_IRQHandler,                   /* USART4                                  */
    USART5_IRQHandler,                   /* USART5                                  */
    TMR6_DAC_GLOBAL_IRQHandler,          /* TMR6 & DAC                              */
    TMR7_GLOBAL_IRQHandler,              /* TMR7                                    */
    DMA2_Channel1_IRQHandler,            /* DMA2 Channel 1                          */
    DMA2_Channel2_IRQHandler,            /* DMA2 Channel 2                          */
    DMA2_Channel3_IRQHandler,            /* DMA2 Channel 3                          */
    DMA2_Channel4_IRQHandler,            /* DMA2 Channel 4                          */
    DMA2_Channel5_IRQHandler,            /* DMA2 Channel 5                          */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    CAN2_TX_IRQHandler,                  /* CAN2 TX                                 */
    CAN2_RX0_IRQHandler,                 /* CAN2 RX0                                */
    CAN2_RX1_IRQHandler,                 /* CAN2 RX1                                */
    CAN2_SE_IRQHandler,                  /* CAN2 SE                                 */
    OTGFS1_IRQHandler,                   /* OTGFS1                                  */
    DMA2_Channel6_IRQHandler,            /* DMA2 Channel 6                          */
    DMA2_Channel7_IRQHandler,            /* DMA2 Channel 7                          */
    0,                                   /* Reserved                                */
    USART6_IRQHandler,                   /* USART6                                  */
    I2C3_EVT_IRQHandler,                 /* I2C3 Event                              */
    I2C3_ERR_IRQHandler,                 /* I2C3 Error                              */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    FPU_IRQHandler,                      /* FPU                                     */
    USART7_IRQHandler,                   /* USART7                                  */
    USART8_IRQHandler,                   /* USART8                                  */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    DMAMUX_IRQHandler,                   /* DMAMUX                                  */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    0,                                   /* Reserved                                */
    ACC_IRQHandler                       /* ACC                                     */
};

#pragma weak WWDT_IRQHandler = Default_Handler
#pragma weak PVM_IRQHandler = Default_Handler
#pragma weak TAMP_STAMP_IRQHandler = Default_Handler
#pragma weak ERTC_WKUP_IRQHandler = Default_Handler
#pragma weak FLASH_IRQHandler = Default_Handler
#pragma weak CRM_IRQHandler = Default_Handler
#pragma weak EXINT0_IRQHandler = Default_Handler
#pragma weak EXINT1_IRQHandler = Default_Handler
#pragma weak EXINT2_IRQHandler = Default_Handler
#pragma weak EXINT3_IRQHandler = Default_Handler
#pragma weak EXINT4_IRQHandler = Default_Handler
#pragma weak DMA1_Channel1_IRQHandler = Default_Handler
#pragma weak DMA1_Channel2_IRQHandler = Default_Handler
#pragma weak DMA1_Channel3_IRQHandler = Default_Handler
#pragma weak DMA1_Channel4_IRQHandler = Default_Handler
#pragma weak DMA1_Channel5_IRQHandler = Default_Handler
#pragma weak DMA1_Channel6_IRQHandler = Default_Handler
#pragma weak DMA1_Channel7_IRQHandler = Default_Handler
#pragma weak ADC1_IRQHandler = Default_Handler
#pragma weak CAN1_TX_IRQHandler = Default_Handler
#pragma weak CAN1_RX0_IRQHandler = Default_Handler
#pragma weak CAN1_RX1_IRQHandler = Default_Handler
#pragma weak CAN1_SE_IRQHandler = Default_Handler
#pragma weak EXINT9_5_IRQHandler = Default_Handler
#pragma weak TMR1_BRK_TMR9_IRQHandler = Default_Handler
#pragma weak TMR1_OVF_TMR10_IRQHandler = Default_Handler
#pragma weak TMR1_TRG_HALL_TMR11_IRQHandler = Default_Handler
#pragma weak TMR1_CH_IRQHandler = Default_Handler
#pragma weak TMR2_GLOBAL_IRQHandler = Default_Handler
#pragma weak TMR3_GLOBAL_IRQHandler = Default_Handler
#pragma weak TMR4_GLOBAL_IRQHandler = Default_Handler
#pragma weak I2C1_EVT_IRQHandler = Default_Handler
#pragma weak I2C1_ERR_IRQHandler = Default_Handler
#pragma weak I2C2_EVT_IRQHandler = Default_Handler
#pragma weak I2C2_ERR_IRQHandler = Default_Handler
#pragma weak SPI1_IRQHandler = Default_Handler
#pragma weak SPI2_IRQHandler = Default_Handler
#pragma weak USART1_IRQHandler = Default_Handler
#pragma weak USART2_IRQHandler = Default_Handler
#pragma weak USART3_IRQHandler = Default_Handler
#pragma weak EXINT15_10_IRQHandler = Default_Handler
#pragma weak ERTCAlarm_IRQHandler = Default_Handler
#pragma weak OTGFS1_WKUP_IRQHandler = Default_Handler
#pragma weak TMR12_GLOBAL_IRQHandler = Default_Handler
#pragma weak TMR13_GLOBAL_IRQHandler = Default_Handler
#pragma weak TMR14_GLOBAL_IRQHandler = Default_Handler
#pragma weak SPI3_IRQHandler = Default_Handler
#pragma weak USART4_IRQHandler = Default_Handler
#pragma weak USART5_IRQHandler = Default_Handler
#pragma weak TMR6_DAC_GLOBAL_IRQHandler = Default_Handler
#pragma weak TMR7_GLOBAL_IRQHandler = Default_Handler
#pragma weak DMA2_Channel1_IRQHandler = Default_Handler
#pragma weak DMA2_Channel2_IRQHandler = Default_Handler
#pragma weak DMA2_Channel3_IRQHandler = Default_Handler
#pragma weak DMA2_Channel4_IRQHandler = Default_Handler
#pragma weak DMA2_Channel5_IRQHandler = Default_Handler
#pragma weak CAN2_TX_IRQHandler = Default_Handler
#pragma weak CAN2_RX0_IRQHandler = Default_Handler
#pragma weak CAN2_RX1_IRQHandler = Default_Handler
#pragma weak CAN2_SE_IRQHandler = Default_Handler
#pragma weak OTGFS1_IRQHandler = Default_Handler
#pragma weak DMA2_Channel6_IRQHandler = Default_Handler
#pragma weak DMA2_Channel7_IRQHandler = Default_Handler
#pragma weak USART6_IRQHandler = Default_Handler
#pragma weak I2C3_EVT_IRQHandler = Default_Handler
#pragma weak I2C3_ERR_IRQHandler = Default_Handler
#pragma weak FPU_IRQHandler = Default_Handler
#pragma weak USART7_IRQHandler = Default_Handler
#pragma weak USART8_IRQHandler = Default_Handler
#pragma weak DMAMUX_IRQHandler = Default_Handler
#pragma weak ACC_IRQHandler = Default_Handler