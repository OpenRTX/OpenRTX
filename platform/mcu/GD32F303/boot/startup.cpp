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
 * startup.c
 * GD32F30x C startup.
 * Supports interrupt handlers in C without extern "C"
 * Developed by Terraneo Federico, based on ST startup code.
 * Additionally modified to boot Miosix.
 */

#include "gd32f30x.h"
#include <string.h>
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

extern "C" void WWDGT_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void LVD_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void Tamper_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void RTC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void FMC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void RCU_CTC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void EXTI0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void EXTI1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void EXTI2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void EXTI3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void EXTI4_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void DMA0_Channel0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void DMA0_Channel1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void DMA0_Channel2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void DMA0_Channel3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void DMA0_Channel4_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void DMA0_Channel5_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void DMA0_Channel6_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void ADC0_1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void USBD_HP_CAN0_TX_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void USBD_LP_CAN0_RX0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void CAN0_RX1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void CAN0_EWMC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void EXTI5_9_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void TIMER0_BRK_TIMER8_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void TIMER0_UP_TIMER9_IRQHandler();
extern "C" void TIMER0_TRG_CMT_TIMER10_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void TIMER0_Channel_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void TIMER1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void TIMER2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void TIMER3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void I2C0_EV_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void I2C0_ER_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void I2C1_EV_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void I2C1_ER_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void SPI0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void SPI1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void USART0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void USART1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void USART2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void EXTI10_15_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void RTC_Alarm_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void USBD_WKUP_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void TIMER7_BRK_TIMER11_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void TIMER7_UP_TIMER12_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void TIMER7_TRG_CMT_TIMER13_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void TIMER7_Channel_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void ADC2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void EXMC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void SDIO_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void TIMER4_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void SPI2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void UART3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void UART4_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void TIMER5_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void TIMER6_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void DMA1_Channel0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void DMA1_Channel1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void DMA1_Channel2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
extern "C" void DMA1_Channel3_4_IRQHandler() __attribute__((weak, alias("Default_Handler")));

// Stack top, defined in the linker script
extern char _main_stack_top asm("_main_stack_top");

// Interrupt vectors, must be placed @ address 0x00000000
//The extern declaration is required otherwise g++ optimizes it out
extern void (* const __Vectors[])();
void (* const __Vectors[])() __attribute__ ((section(".isr_vector"))) =
{
    reinterpret_cast<void (*)()>(&_main_stack_top),/* Stack pointer*/
    Reset_Handler,                   // Reset handler
    NMI_Handler,                     // NMI handler
    HardFault_Handler,               // Hard fault handler
    MemManage_Handler,               // MPU fault handler
    BusFault_Handler,                // Bus fault handler
    UsageFault_Handler,              // Usage fault handler
    0, 0, 0, 0,                      // Reserved
    SVC_Handler,                     // SVCall handler
    DebugMon_Handler,                // Debug monitor handler
    0,                               // Reserved
    PendSV_Handler,                  // PendSV handler
    SysTick_Handler,                 // SysTick handler
    WWDGT_IRQHandler,                // Window Watchdog Timer
    LVD_IRQHandler,                  // LVD through EXTI Line detect
    Tamper_IRQHandler,               // Tamper
    RTC_IRQHandler,                  // RTC through EXTI Line
    FMC_IRQHandler,                  // FMC
    RCU_CTC_IRQHandler,              // RCU and CTC
    EXTI0_IRQHandler,                // EXTI Line 0
    EXTI1_IRQHandler,                // EXTI Line 1
    EXTI2_IRQHandler,                // EXTI Line 2
    EXTI3_IRQHandler,                // EXTI Line 3
    EXTI4_IRQHandler,                // EXTI Line 4
    DMA0_Channel0_IRQHandler,        // DMA0 Channel0
    DMA0_Channel1_IRQHandler,        // DMA0 Channel1
    DMA0_Channel2_IRQHandler,        // DMA0 Channel2
    DMA0_Channel3_IRQHandler,        // DMA0 Channel3
    DMA0_Channel4_IRQHandler,        // DMA0 Channel4
    DMA0_Channel5_IRQHandler,        // DMA0 Channel5
    DMA0_Channel6_IRQHandler,        // DMA0 Channel6
    ADC0_1_IRQHandler,               // ADC0 and ADC1
    USBD_HP_CAN0_TX_IRQHandler,      // USBD HP and CAN0 TX
    USBD_LP_CAN0_RX0_IRQHandler,     // USBD LP and CAN0 RX0
    CAN0_RX1_IRQHandler,             // CAN0 RX1
    CAN0_EWMC_IRQHandler,            // CAN0 EWMC
    EXTI5_9_IRQHandler,              // EXTI5 to EXTI9
    TIMER0_BRK_TIMER8_IRQHandler,    // TIMER0 Break and TIMER8
    TIMER0_UP_TIMER9_IRQHandler,     // TIMER0 Update and TIMER9
    TIMER0_TRG_CMT_TIMER10_IRQHandler, // TIMER0 Trigger and Commutation and TIMER10
    TIMER0_Channel_IRQHandler,       // TIMER0 Channel Capture Compare
    TIMER1_IRQHandler,               // TIMER1
    TIMER2_IRQHandler,               // TIMER2
    TIMER3_IRQHandler,               // TIMER3
    I2C0_EV_IRQHandler,              // I2C0 Event
    I2C0_ER_IRQHandler,              // I2C0 Error
    I2C1_EV_IRQHandler,              // I2C1 Event
    I2C1_ER_IRQHandler,              // I2C1 Error
    SPI0_IRQHandler,                 // SPI0
    SPI1_IRQHandler,                 // SPI1
    USART0_IRQHandler,               // USART0
    USART1_IRQHandler,               // USART1
    USART2_IRQHandler,               // USART2
    EXTI10_15_IRQHandler,            // EXTI10 to EXTI15
    RTC_Alarm_IRQHandler,            // RTC Alarm
    USBD_WKUP_IRQHandler,            // USBD Wakeup
    TIMER7_BRK_TIMER11_IRQHandler,   // TIMER7 Break and TIMER11
    TIMER7_UP_TIMER12_IRQHandler,    // TIMER7 Update and TIMER12
    TIMER7_TRG_CMT_TIMER13_IRQHandler, // TIMER7 Trigger and Commutation and TIMER13
    TIMER7_Channel_IRQHandler,       // TIMER7 Channel Capture Compare
    ADC2_IRQHandler,                 // ADC2
    EXMC_IRQHandler,                 // EXMC
    SDIO_IRQHandler,                 // SDIO
    TIMER4_IRQHandler,               // TIMER4
    SPI2_IRQHandler,                 // SPI2
    UART3_IRQHandler,                // UART3
    UART4_IRQHandler,                // UART4
    TIMER5_IRQHandler,               // TIMER5
    TIMER6_IRQHandler,               // TIMER6
    DMA1_Channel0_IRQHandler,        // DMA1 Channel0
    DMA1_Channel1_IRQHandler,        // DMA1 Channel1
    DMA1_Channel2_IRQHandler,        // DMA1 Channel2
    DMA1_Channel3_4_IRQHandler,      // DMA1 Channel3 and Channel4
};