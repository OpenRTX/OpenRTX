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
#include <gd32f3x0.h>

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
// void __attribute__((weak, alias("Default_Handler"))) Reset_Handler(void);                     //; Reset Handler
// void __attribute__((weak, alias("Default_Handler"))) NMI_Handler(void);                       //; NMI Handler
// void __attribute__((weak, alias("Default_Handler"))) HardFault_Handler(void);                 //; Hard Fault Handler
// void __attribute__((weak, alias("Default_Handler"))) MemManage_Handler(void);                 //; MPU Fault Handler
// void __attribute__((weak, alias("Default_Handler"))) BusFault_Handler(void);                  //; Bus Fault Handler
// void __attribute__((weak, alias("Default_Handler"))) UsageFault_Handler(void);                //; Usage Fault Handler
// void __attribute__((weak, alias("Default_Handler"))) SVC_Handler(void);                       //; SVCall Handler
// void __attribute__((weak, alias("Default_Handler"))) DebugMon_Handler(void);                  //; Debug Monitor Handler
// void __attribute__((weak, alias("Default_Handler"))) PendSV_Handler(void);                    //; PendSV Handler
// void __attribute__((weak, alias("Default_Handler"))) SysTick_Handler(void);                   //; SysTick Handler                     
extern "C" void __attribute__((weak, alias("Default_Handler"))) WWDGT_IRQHandler(void);                  //; 16:Window Watchdog Timer
extern "C" void __attribute__((weak, alias("Default_Handler"))) LVD_IRQHandler(void);                    //; 17:LVD through EXTI Line detect
extern "C" void __attribute__((weak, alias("Default_Handler"))) RTC_IRQHandler(void);                    //; 18:RTC through EXTI Line
extern "C" void __attribute__((weak, alias("Default_Handler"))) FMC_IRQHandler(void);                    //; 19:FMC
extern "C" void __attribute__((weak, alias("Default_Handler"))) RCU_CTC_IRQHandler(void);                //; 20:RCU and CTC
extern "C" void __attribute__((weak, alias("Default_Handler"))) EXTI0_1_IRQHandler(void);                //; 21:EXTI Line 0 and EXTI Line 1
extern "C" void __attribute__((weak, alias("Default_Handler"))) EXTI2_3_IRQHandler(void);                //; 22:EXTI Line 2 and EXTI Line 3
extern "C" void __attribute__((weak, alias("Default_Handler"))) EXTI4_15_IRQHandler(void);               //; 23:EXTI Line 4 to EXTI Line 15
extern "C" void __attribute__((weak, alias("Default_Handler"))) TSI_IRQHandler(void);                    //; 24:TSI
extern "C" void __attribute__((weak, alias("Default_Handler"))) DMA_Channel0_IRQHandler(void);           //; 25:DMA Channel 0 
extern "C" void __attribute__((weak, alias("Default_Handler"))) DMA_Channel1_2_IRQHandler(void);         //; 26:DMA Channel 1 and DMA Channel 2
extern "C" void __attribute__((weak, alias("Default_Handler"))) DMA_Channel3_4_IRQHandler(void);         //; 27:DMA Channel 3 and DMA Channel 4
extern "C" void __attribute__((weak, alias("Default_Handler"))) ADC_CMP_IRQHandler(void);                //; 28:ADC and Comparator 0-1
extern "C" void __attribute__((weak, alias("Default_Handler"))) TIMER0_BRK_UP_TRG_COM_IRQHandler(void);  //; 29:TIMER0 Break,Update,Trigger and Commutation
extern "C" void __attribute__((weak, alias("Default_Handler"))) TIMER0_Channel_IRQHandler(void);         //; 30:TIMER0 Channel Capture Compare
extern "C" void __attribute__((weak, alias("Default_Handler"))) TIMER1_IRQHandler(void);                 //; 31:TIMER1
extern "C" void __attribute__((weak, alias("Default_Handler"))) TIMER2_IRQHandler(void);                 //; 32:TIMER2
extern "C" void __attribute__((weak, alias("Default_Handler"))) TIMER5_DAC_IRQHandler(void);             //; 33:TIMER5 and DAC
extern "C" void __attribute__((weak, alias("Default_Handler"))) TIMER13_IRQHandler(void);                //; 35:TIMER13
extern "C" void __attribute__((weak, alias("Default_Handler"))) TIMER14_IRQHandler(void);                //; 36:TIMER14
extern "C" void __attribute__((weak, alias("Default_Handler"))) TIMER15_IRQHandler(void);                //; 37:TIMER15
extern "C" void __attribute__((weak, alias("Default_Handler"))) TIMER16_IRQHandler(void);                //; 38:TIMER16
extern "C" void __attribute__((weak, alias("Default_Handler"))) I2C0_EV_IRQHandler(void);                //; 39:I2C0 Event
extern "C" void __attribute__((weak, alias("Default_Handler"))) I2C1_EV_IRQHandler(void);                //; 40:I2C1 Event
extern "C" void __attribute__((weak, alias("Default_Handler"))) SPI0_IRQHandler(void);                   //; 41:SPI0
extern "C" void __attribute__((weak, alias("Default_Handler"))) SPI1_IRQHandler(void);                   //; 42:SPI1
extern "C" void __attribute__((weak, alias("Default_Handler"))) USART0_IRQHandler(void);                 //; 43:USART0
extern "C" void __attribute__((weak, alias("Default_Handler"))) USART1_IRQHandler(void);                 //; 44:USART1
extern "C" void __attribute__((weak, alias("Default_Handler"))) CEC_IRQHandler(void);                    //; 46:CEC
extern "C" void __attribute__((weak, alias("Default_Handler"))) I2C0_ER_IRQHandler(void);                //; 48:I2C0 Error
extern "C" void __attribute__((weak, alias("Default_Handler"))) I2C1_ER_IRQHandler(void);                //; 50:I2C1 Error
extern "C" void __attribute__((weak, alias("Default_Handler"))) USBFS_WKUP_IRQHandler(void);             //; 58:USBFS Wakeup
extern "C" void __attribute__((weak, alias("Default_Handler"))) DMA_Channel5_6_IRQHandler(void);         //; 64:DMA Channel5 and Channel6 
extern "C" void __attribute__((weak, alias("Default_Handler"))) USBFS_IRQHandler(void);                  //; 83:USBFS

//Stack top, defined in the linker script
extern char _main_stack_top asm("_main_stack_top");

//Interrupt vectors, must be placed @ address 0x00000000
//The extern declaration is required otherwise g++ optimizes it out
extern void (* const __Vectors[])();
void (* const __Vectors[])() __attribute__ ((section(".isr_vector"))) =
{
    reinterpret_cast<void (*)()>(&_main_stack_top),/* Stack pointer*/
    Reset_Handler,                     //; Reset Handler
    NMI_Handler,                       //; NMI Handler
    HardFault_Handler,                 //; Hard Fault Handler
    MemManage_Handler,                 //; MPU Fault Handler
    BusFault_Handler,                  //; Bus Fault Handler
    UsageFault_Handler,                //; Usage Fault Handler
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    SVC_Handler,                       //; SVCall Handler
    DebugMon_Handler,                  //; Debug Monitor Handler
    0,                                 //; Reserved
    PendSV_Handler,                    //; PendSV Handler
    SysTick_Handler,                   //; SysTick Handler

                                        /*, external interrupts handler */
    WWDGT_IRQHandler,                  //; 16:Window Watchdog Timer
    LVD_IRQHandler,                    //; 17:LVD through EXTI Line detect
    RTC_IRQHandler,                    //; 18:RTC through EXTI Line
    FMC_IRQHandler,                    //; 19:FMC
    RCU_CTC_IRQHandler,                //; 20:RCU and CTC
    EXTI0_1_IRQHandler,                //; 21:EXTI Line 0 and EXTI Line 1
    EXTI2_3_IRQHandler,                //; 22:EXTI Line 2 and EXTI Line 3
    EXTI4_15_IRQHandler,               //; 23:EXTI Line 4 to EXTI Line 15
    TSI_IRQHandler,                    //; 24:TSI
    DMA_Channel0_IRQHandler,           //; 25:DMA Channel 0 
    DMA_Channel1_2_IRQHandler,         //; 26:DMA Channel 1 and DMA Channel 2
    DMA_Channel3_4_IRQHandler,         //; 27:DMA Channel 3 and DMA Channel 4
    ADC_CMP_IRQHandler,                //; 28:ADC and Comparator 0-1
    TIMER0_BRK_UP_TRG_COM_IRQHandler,  //; 29:TIMER0 Break,Update,Trigger and Commutation
    TIMER0_Channel_IRQHandler,         //; 30:TIMER0 Channel Capture Compare
    TIMER1_IRQHandler,                 //; 31:TIMER1
    TIMER2_IRQHandler,                 //; 32:TIMER2
    TIMER5_DAC_IRQHandler,             //; 33:TIMER5 and DAC
    0,                                 //; Reserved
    TIMER13_IRQHandler,                //; 35:TIMER13
    TIMER14_IRQHandler,                //; 36:TIMER14
    TIMER15_IRQHandler,                //; 37:TIMER15
    TIMER16_IRQHandler,                //; 38:TIMER16
    I2C0_EV_IRQHandler,                //; 39:I2C0 Event
    I2C1_EV_IRQHandler,                //; 40:I2C1 Event
    SPI0_IRQHandler,                   //; 41:SPI0
    SPI1_IRQHandler,                   //; 42:SPI1
    USART0_IRQHandler,                 //; 43:USART0
    USART1_IRQHandler,                 //; 44:USART1
    0,                                 //; Reserved
    CEC_IRQHandler,                    //; 46:CEC
    0,                                 //; Reserved
    I2C0_ER_IRQHandler,                //; 48:I2C0 Error
    0,                                 //; Reserved
    I2C1_ER_IRQHandler,                //; 50:I2C1 Error
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    USBFS_WKUP_IRQHandler,             //; 58:USBFS Wakeup
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    DMA_Channel5_6_IRQHandler,         //; 64:DMA Channel5 and Channel6 
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    0,                                 //; Reserved
    USBFS_IRQHandler,                  //; 83:USBFS
};

// #pragma weak WWDT_IRQHandler = Default_Handler
// #pragma weak PVM_IRQHandler = Default_Handler
// #pragma weak ERTC_IRQHandler = Default_Handler
// #pragma weak FLASH_IRQHandler = Default_Handler
// #pragma weak CRM_IRQHandler = Default_Handler
// #pragma weak EXINT1_0_IRQHandler = Default_Handler
// #pragma weak EXINT3_2_IRQHandler = Default_Handler
// #pragma weak EXINT15_4_IRQHandler = Default_Handler
// #pragma weak DMA1_Channel1_IRQHandler = Default_Handler
// #pragma weak DMA1_Channel3_2_IRQHandler = Default_Handler
// #pragma weak DMA1_Channel5_4_IRQHandler = Default_Handler
// #pragma weak ADC1_CMP_IRQHandler = Default_Handler
// #pragma weak TMR1_BRK_OVF_TRG_HALL_IRQHandler = Default_Handler
// #pragma weak TMR1_CH_IRQHandler = Default_Handler
// #pragma weak TMR3_GLOBAL_IRQHandler = Default_Handler
// #pragma weak TMR6_GLOBAL_IRQHandler = Default_Handler
// #pragma weak TMR14_GLOBAL_IRQHandler = Default_Handler
// #pragma weak TMR15_GLOBAL_IRQHandler = Default_Handler
// // #pragma weak TMR16_GLOBAL_IRQHandler = Default_Handler
// #pragma weak TMR17_GLOBAL_IRQHandler = Default_Handler
// #pragma weak I2C1_EVT_IRQHandler = Default_Handler
// #pragma weak I2C2_EVT_IRQHandler = Default_Handler
// #pragma weak SPI1_IRQHandler = Default_Handler
// #pragma weak SPI2_IRQHandler = Default_Handler
// #pragma weak USART1_IRQHandler = Default_Handler
// #pragma weak USART2_IRQHandler = Default_Handler
// #pragma weak I2C1_ERR_IRQHandler = Default_Handler
// #pragma weak I2C2_ERR_IRQHandler = Default_Handler
