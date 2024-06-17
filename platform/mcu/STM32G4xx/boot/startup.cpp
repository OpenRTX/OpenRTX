/***************************************************************************
 *   Copyright (C) 2024 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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

#include "interfaces/arch_registers.h"
#include "kernel/stage_2_boot.h"
#include "core/interrupts.h"
#include <string.h>

/**
 * Called by Reset_Handler, performs initialization and calls main.
 * Never returns.
 */
void program_startup() __attribute__((noreturn));
void program_startup()
{
    //Cortex M4 core appears to get out of reset with interrupts already enabled
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
void __attribute__((weak)) WWDG_IRQHandler();
void __attribute__((weak)) PVD_PVM_IRQHandler();
void __attribute__((weak)) RTC_TAMP_LSECSS_IRQHandler();
void __attribute__((weak)) RTC_WKUP_IRQHandler();
void __attribute__((weak)) FLASH_IRQHandler();
void __attribute__((weak)) RCC_IRQHandler();
void __attribute__((weak)) EXTI0_IRQHandler();
void __attribute__((weak)) EXTI1_IRQHandler();
void __attribute__((weak)) EXTI2_IRQHandler();
void __attribute__((weak)) EXTI3_IRQHandler();
void __attribute__((weak)) EXTI4_IRQHandler();
void __attribute__((weak)) DMA1_Channel1_IRQHandler();
void __attribute__((weak)) DMA1_Channel2_IRQHandler();
void __attribute__((weak)) DMA1_Channel3_IRQHandler();
void __attribute__((weak)) DMA1_Channel4_IRQHandler();
void __attribute__((weak)) DMA1_Channel5_IRQHandler();
void __attribute__((weak)) DMA1_Channel6_IRQHandler();
void __attribute__((weak)) DMA1_Channel7_IRQHandler();
void __attribute__((weak)) ADC1_2_IRQHandler();
void __attribute__((weak)) USB_HP_IRQHandler();
void __attribute__((weak)) USB_LP_IRQHandler();
void __attribute__((weak)) FDCAN1_IT0_IRQHandler();
void __attribute__((weak)) FDCAN1_IT1_IRQHandler();
void __attribute__((weak)) EXTI9_5_IRQHandler();
void __attribute__((weak)) TIM1_BRK_TIM15_IRQHandler();
void __attribute__((weak)) TIM1_UP_TIM16_IRQHandler();
void __attribute__((weak)) TIM1_TRG_COM_TIM17_IRQHandler();
void __attribute__((weak)) TIM1_CC_IRQHandler();
void __attribute__((weak)) TIM2_IRQHandler();
void __attribute__((weak)) TIM3_IRQHandler();
void __attribute__((weak)) TIM4_IRQHandler();
void __attribute__((weak)) I2C1_EV_IRQHandler();
void __attribute__((weak)) I2C1_ER_IRQHandler();
void __attribute__((weak)) I2C2_EV_IRQHandler();
void __attribute__((weak)) I2C2_ER_IRQHandler();
void __attribute__((weak)) SPI1_IRQHandler();
void __attribute__((weak)) SPI2_IRQHandler();
void __attribute__((weak)) USART1_IRQHandler();
void __attribute__((weak)) USART2_IRQHandler();
void __attribute__((weak)) USART3_IRQHandler();
void __attribute__((weak)) EXTI15_10_IRQHandler();
void __attribute__((weak)) RTC_Alarm_IRQHandler();
void __attribute__((weak)) USBWakeUp_IRQHandler();
void __attribute__((weak)) TIM8_BRK_IRQHandler();
void __attribute__((weak)) TIM8_UP_IRQHandler();
void __attribute__((weak)) TIM8_TRG_COM_IRQHandler();
void __attribute__((weak)) TIM8_CC_IRQHandler();
void __attribute__((weak)) ADC3_IRQHandler();
void __attribute__((weak)) FMC_IRQHandler();
void __attribute__((weak)) LPTIM1_IRQHandler();
void __attribute__((weak)) TIM5_IRQHandler();
void __attribute__((weak)) SPI3_IRQHandler();
void __attribute__((weak)) UART4_IRQHandler();
void __attribute__((weak)) UART5_IRQHandler();
void __attribute__((weak)) TIM6_DAC_IRQHandler();
void __attribute__((weak)) TIM7_DAC_IRQHandler();
void __attribute__((weak)) DMA2_Channel1_IRQHandler();
void __attribute__((weak)) DMA2_Channel2_IRQHandler();
void __attribute__((weak)) DMA2_Channel3_IRQHandler();
void __attribute__((weak)) DMA2_Channel4_IRQHandler();
void __attribute__((weak)) DMA2_Channel5_IRQHandler();
void __attribute__((weak)) ADC4_IRQHandler();
void __attribute__((weak)) ADC5_IRQHandler();
void __attribute__((weak)) UCPD1_IRQHandler();
void __attribute__((weak)) COMP1_2_3_IRQHandler();
void __attribute__((weak)) COMP4_5_6_IRQHandler();
void __attribute__((weak)) COMP7_IRQHandler();
void __attribute__((weak)) HRTIM1_Master_IRQHandler();
void __attribute__((weak)) HRTIM1_TIMA_IRQHandler();
void __attribute__((weak)) HRTIM1_TIMB_IRQHandler();
void __attribute__((weak)) HRTIM1_TIMC_IRQHandler();
void __attribute__((weak)) HRTIM1_TIMD_IRQHandler();
void __attribute__((weak)) HRTIM1_TIME_IRQHandler();
void __attribute__((weak)) HRTIM1_FLT_IRQHandler();
void __attribute__((weak)) HRTIM1_TIMF_IRQHandler();
void __attribute__((weak)) CRS_IRQHandler();
void __attribute__((weak)) SAI1_IRQHandler();
void __attribute__((weak)) TIM20_BRK_IRQHandler();
void __attribute__((weak)) TIM20_UP_IRQHandler();
void __attribute__((weak)) TIM20_TRG_COM_IRQHandler();
void __attribute__((weak)) TIM20_CC_IRQHandler();
void __attribute__((weak)) FPU_IRQHandler();
void __attribute__((weak)) I2C4_EV_IRQHandler();
void __attribute__((weak)) I2C4_ER_IRQHandler();
void __attribute__((weak)) SPI4_IRQHandler();
void __attribute__((weak)) FDCAN2_IT0_IRQHandler();
void __attribute__((weak)) FDCAN2_IT1_IRQHandler();
void __attribute__((weak)) FDCAN3_IT0_IRQHandler();
void __attribute__((weak)) FDCAN3_IT1_IRQHandler();
void __attribute__((weak)) RNG_IRQHandler();
void __attribute__((weak)) LPUART1_IRQHandler();
void __attribute__((weak)) I2C3_EV_IRQHandler();
void __attribute__((weak)) I2C3_ER_IRQHandler();
void __attribute__((weak)) DMAMUX_OVR_IRQHandler();
void __attribute__((weak)) QUADSPI_IRQHandler();
void __attribute__((weak)) DMA1_Channel8_IRQHandler();
void __attribute__((weak)) DMA2_Channel6_IRQHandler();
void __attribute__((weak)) DMA2_Channel7_IRQHandler();
void __attribute__((weak)) DMA2_Channel8_IRQHandler();
void __attribute__((weak)) CORDIC_IRQHandler();
void __attribute__((weak)) FMAC_IRQHandler();

//Stack top, defined in the linker script
extern char _main_stack_top asm("_main_stack_top");

//Interrupt vectors, must be placed @ address 0x00000000
//The extern declaration is required otherwise g++ optimizes it out
extern void (* const __Vectors[])();
void (* const __Vectors[])() __attribute__ ((section(".isr_vector"))) =
{
    reinterpret_cast<void (*)()>(&_main_stack_top),/* Stack pointer*/
    Reset_Handler,              /* Reset Handler */
    NMI_Handler,                /* NMI Handler */
    HardFault_Handler,          /* Hard Fault Handler */
    MemManage_Handler,          /* MPU Fault Handler */
    BusFault_Handler,           /* Bus Fault Handler */
    UsageFault_Handler,         /* Usage Fault Handler */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    SVC_Handler,                /* SVCall Handler */
    DebugMon_Handler,           /* Debug Monitor Handler */
    0,                          /* Reserved */
    PendSV_Handler,             /* PendSV Handler */
    SysTick_Handler,            /* SysTick Handler */

    /* External Interrupts */
    WWDG_IRQHandler,
    PVD_PVM_IRQHandler,
    RTC_TAMP_LSECSS_IRQHandler,
    RTC_WKUP_IRQHandler,
    FLASH_IRQHandler,
    RCC_IRQHandler,
    EXTI0_IRQHandler,
    EXTI1_IRQHandler,
    EXTI2_IRQHandler,
    EXTI3_IRQHandler,
    EXTI4_IRQHandler,
    DMA1_Channel1_IRQHandler,
    DMA1_Channel2_IRQHandler,
    DMA1_Channel3_IRQHandler,
    DMA1_Channel4_IRQHandler,
    DMA1_Channel5_IRQHandler,
    DMA1_Channel6_IRQHandler,
    DMA1_Channel7_IRQHandler,
    ADC1_2_IRQHandler,
    USB_HP_IRQHandler,
    USB_LP_IRQHandler,
    FDCAN1_IT0_IRQHandler,
    FDCAN1_IT1_IRQHandler,
    EXTI9_5_IRQHandler,
    TIM1_BRK_TIM15_IRQHandler,
    TIM1_UP_TIM16_IRQHandler,
    TIM1_TRG_COM_TIM17_IRQHandler,
    TIM1_CC_IRQHandler,
    TIM2_IRQHandler,
    TIM3_IRQHandler,
    TIM4_IRQHandler,
    I2C1_EV_IRQHandler,
    I2C1_ER_IRQHandler,
    I2C2_EV_IRQHandler,
    I2C2_ER_IRQHandler,
    SPI1_IRQHandler,
    SPI2_IRQHandler,
    USART1_IRQHandler,
    USART2_IRQHandler,
    USART3_IRQHandler,
    EXTI15_10_IRQHandler,
    RTC_Alarm_IRQHandler,
    USBWakeUp_IRQHandler,
    TIM8_BRK_IRQHandler,
    TIM8_UP_IRQHandler,
    TIM8_TRG_COM_IRQHandler,
    TIM8_CC_IRQHandler,
    ADC3_IRQHandler,
    FMC_IRQHandler,
    LPTIM1_IRQHandler,
    TIM5_IRQHandler,
    SPI3_IRQHandler,
    UART4_IRQHandler,
    UART5_IRQHandler,
    TIM6_DAC_IRQHandler,
    TIM7_DAC_IRQHandler,
    DMA2_Channel1_IRQHandler,
    DMA2_Channel2_IRQHandler,
    DMA2_Channel3_IRQHandler,
    DMA2_Channel4_IRQHandler,
    DMA2_Channel5_IRQHandler,
    ADC4_IRQHandler,
    ADC5_IRQHandler,
    UCPD1_IRQHandler,
    COMP1_2_3_IRQHandler,
    COMP4_5_6_IRQHandler,
    COMP7_IRQHandler,
    HRTIM1_Master_IRQHandler,
    HRTIM1_TIMA_IRQHandler,
    HRTIM1_TIMB_IRQHandler,
    HRTIM1_TIMC_IRQHandler,
    HRTIM1_TIMD_IRQHandler,
    HRTIM1_TIME_IRQHandler,
    HRTIM1_FLT_IRQHandler,
    HRTIM1_TIMF_IRQHandler,
    CRS_IRQHandler,
    SAI1_IRQHandler,
    TIM20_BRK_IRQHandler,
    TIM20_UP_IRQHandler,
    TIM20_TRG_COM_IRQHandler,
    TIM20_CC_IRQHandler,
    FPU_IRQHandler,
    I2C4_EV_IRQHandler,
    I2C4_ER_IRQHandler,
    SPI4_IRQHandler,
    0,
    FDCAN2_IT0_IRQHandler,
    FDCAN2_IT1_IRQHandler,
    FDCAN3_IT0_IRQHandler,
    FDCAN3_IT1_IRQHandler,
    RNG_IRQHandler,
    LPUART1_IRQHandler,
    I2C3_EV_IRQHandler,
    I2C3_ER_IRQHandler,
    DMAMUX_OVR_IRQHandler,
    QUADSPI_IRQHandler,
    DMA1_Channel8_IRQHandler,
    DMA2_Channel6_IRQHandler,
    DMA2_Channel7_IRQHandler,
    DMA2_Channel8_IRQHandler,
    CORDIC_IRQHandler,
    FMAC_IRQHandler,
};

#pragma weak WWDG_IRQHandler = Default_Handler
#pragma weak PVD_PVM_IRQHandler = Default_Handler
#pragma weak RTC_TAMP_LSECSS_IRQHandler = Default_Handler
#pragma weak RTC_WKUP_IRQHandler = Default_Handler
#pragma weak FLASH_IRQHandler = Default_Handler
#pragma weak RCC_IRQHandler = Default_Handler
#pragma weak EXTI0_IRQHandler = Default_Handler
#pragma weak EXTI1_IRQHandler = Default_Handler
#pragma weak EXTI2_IRQHandler = Default_Handler
#pragma weak EXTI3_IRQHandler = Default_Handler
#pragma weak EXTI4_IRQHandler = Default_Handler
#pragma weak DMA1_Channel1_IRQHandler = Default_Handler
#pragma weak DMA1_Channel2_IRQHandler = Default_Handler
#pragma weak DMA1_Channel3_IRQHandler = Default_Handler
#pragma weak DMA1_Channel4_IRQHandler = Default_Handler
#pragma weak DMA1_Channel5_IRQHandler = Default_Handler
#pragma weak DMA1_Channel6_IRQHandler = Default_Handler
#pragma weak DMA1_Channel7_IRQHandler = Default_Handler
#pragma weak ADC1_2_IRQHandler = Default_Handler
#pragma weak USB_HP_IRQHandler = Default_Handler
#pragma weak USB_LP_IRQHandler = Default_Handler
#pragma weak FDCAN1_IT0_IRQHandler = Default_Handler
#pragma weak FDCAN1_IT1_IRQHandler = Default_Handler
#pragma weak EXTI9_5_IRQHandler = Default_Handler
#pragma weak TIM1_BRK_TIM15_IRQHandler = Default_Handler
#pragma weak TIM1_UP_TIM16_IRQHandler = Default_Handler
#pragma weak TIM1_TRG_COM_TIM17_IRQHandler = Default_Handler
#pragma weak TIM1_CC_IRQHandler = Default_Handler
#pragma weak TIM2_IRQHandler = Default_Handler
#pragma weak TIM3_IRQHandler = Default_Handler
#pragma weak TIM4_IRQHandler = Default_Handler
#pragma weak I2C1_EV_IRQHandler = Default_Handler
#pragma weak I2C1_ER_IRQHandler = Default_Handler
#pragma weak I2C2_EV_IRQHandler = Default_Handler
#pragma weak I2C2_ER_IRQHandler = Default_Handler
#pragma weak SPI1_IRQHandler = Default_Handler
#pragma weak SPI2_IRQHandler = Default_Handler
#pragma weak USART1_IRQHandler = Default_Handler
#pragma weak USART2_IRQHandler = Default_Handler
#pragma weak USART3_IRQHandler = Default_Handler
#pragma weak EXTI15_10_IRQHandler = Default_Handler
#pragma weak RTC_Alarm_IRQHandler = Default_Handler
#pragma weak USBWakeUp_IRQHandler = Default_Handler
#pragma weak TIM8_BRK_IRQHandler = Default_Handler
#pragma weak TIM8_UP_IRQHandler = Default_Handler
#pragma weak TIM8_TRG_COM_IRQHandler = Default_Handler
#pragma weak TIM8_CC_IRQHandler = Default_Handler
#pragma weak ADC3_IRQHandler = Default_Handler
#pragma weak FMC_IRQHandler = Default_Handler
#pragma weak LPTIM1_IRQHandler = Default_Handler
#pragma weak TIM5_IRQHandler = Default_Handler
#pragma weak SPI3_IRQHandler = Default_Handler
#pragma weak UART4_IRQHandler = Default_Handler
#pragma weak UART5_IRQHandler = Default_Handler
#pragma weak TIM6_DAC_IRQHandler = Default_Handler
#pragma weak TIM7_DAC_IRQHandler = Default_Handler
#pragma weak DMA2_Channel1_IRQHandler = Default_Handler
#pragma weak DMA2_Channel2_IRQHandler = Default_Handler
#pragma weak DMA2_Channel3_IRQHandler = Default_Handler
#pragma weak DMA2_Channel4_IRQHandler = Default_Handler
#pragma weak DMA2_Channel5_IRQHandler = Default_Handler
#pragma weak ADC4_IRQHandler = Default_Handler
#pragma weak ADC5_IRQHandler = Default_Handler
#pragma weak UCPD1_IRQHandler = Default_Handler
#pragma weak COMP1_2_3_IRQHandler = Default_Handler
#pragma weak COMP4_5_6_IRQHandler = Default_Handler
#pragma weak COMP7_IRQHandler = Default_Handler
#pragma weak HRTIM1_Master_IRQHandler = Default_Handler
#pragma weak HRTIM1_TIMA_IRQHandler = Default_Handler
#pragma weak HRTIM1_TIMB_IRQHandler = Default_Handler
#pragma weak HRTIM1_TIMC_IRQHandler = Default_Handler
#pragma weak HRTIM1_TIMD_IRQHandler = Default_Handler
#pragma weak HRTIM1_TIME_IRQHandler = Default_Handler
#pragma weak HRTIM1_FLT_IRQHandler = Default_Handler
#pragma weak HRTIM1_TIMF_IRQHandler = Default_Handler
#pragma weak CRS_IRQHandler = Default_Handler
#pragma weak SAI1_IRQHandler = Default_Handler
#pragma weak TIM20_BRK_IRQHandler = Default_Handler
#pragma weak TIM20_UP_IRQHandler = Default_Handler
#pragma weak TIM20_TRG_COM_IRQHandler = Default_Handler
#pragma weak TIM20_CC_IRQHandler = Default_Handler
#pragma weak FPU_IRQHandler = Default_Handler
#pragma weak I2C4_EV_IRQHandler = Default_Handler
#pragma weak I2C4_ER_IRQHandler = Default_Handler
#pragma weak SPI4_IRQHandler = Default_Handler
#pragma weak FDCAN2_IT0_IRQHandler = Default_Handler
#pragma weak FDCAN2_IT1_IRQHandler = Default_Handler
#pragma weak FDCAN3_IT0_IRQHandler = Default_Handler
#pragma weak FDCAN3_IT1_IRQHandler = Default_Handler
#pragma weak RNG_IRQHandler = Default_Handler
#pragma weak LPUART1_IRQHandler = Default_Handler
#pragma weak I2C3_EV_IRQHandler = Default_Handler
#pragma weak I2C3_ER_IRQHandler = Default_Handler
#pragma weak DMAMUX_OVR_IRQHandler = Default_Handler
#pragma weak QUADSPI_IRQHandler = Default_Handler
#pragma weak DMA1_Channel8_IRQHandler = Default_Handler
#pragma weak DMA2_Channel6_IRQHandler = Default_Handler
#pragma weak DMA2_Channel7_IRQHandler = Default_Handler
#pragma weak DMA2_Channel8_IRQHandler = Default_Handler
#pragma weak CORDIC_IRQHandler = Default_Handler
#pragma weak FMAC_IRQHandler = Default_Handler
