/***************************************************************************
 *   Copyright (C) 2020 by Federico Izzo IU2NUO, Niccolò Izzo IU2KIN and   *
 *                         Silvano Seva IU2KWO                             *
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
 

#include <string.h>
#include <stdio.h>
#include "MK22F51212.h"

///< Entry point for system bootstrap after initial configurations.
void systemBootstrap();

void Reset_Handler() __attribute__((__interrupt__, noreturn));
void Reset_Handler()
{
    // Disable interrupts at startup, as the RTOS requires this.
    __disable_irq();

    // Call CMSIS init function, it's safe to do it here.
    // This function initialises VTOR, clock-tree and flash memory wait states.
    SystemInit();

    //These are defined in the linker script
    extern unsigned char _etext asm("_etext");
    extern unsigned char _data asm("_data");
    extern unsigned char _edata asm("_edata");
    extern unsigned char _bss_start asm("_bss_start");
    extern unsigned char _bss_end asm("_bss_end");

    // Initialize .data section, clear .bss section
    unsigned char *etext=&_etext;
    unsigned char *data=&_data;
    unsigned char *edata=&_edata;
    unsigned char *bss_start=&_bss_start;
    unsigned char *bss_end=&_bss_end;

    memcpy(data, etext, edata-data);
    memset(bss_start, 0, bss_end-bss_start);

    SIM->SCGC5 |= 0x3E00;   // Enable GPIO clock

//     // Enable virtual com port (for stdin, stdout and stderr redirection)
//     vcom_init();

    // Set no buffer for stdin, required to make scanf, getchar, ... working
    // correctly
    setvbuf(stdin, NULL, _IONBF, 0);


    // Jump to application code
    systemBootstrap();

    // If main returns loop indefinitely
    for(;;) ;
}

void Default_Handler()
{
    // default handler does nothing
}

void __attribute__((weak)) NMI_Handler();
void __attribute__((weak)) HardFault_Handler();
void __attribute__((weak)) MemManage_Handler();
void __attribute__((weak)) BusFault_Handler();
void __attribute__((weak)) UsageFault_Handler();
void __attribute__((weak)) SVC_Handler();
void __attribute__((weak)) DebugMon_Handler();

void __attribute__((weak)) OS_CPU_PendSVHandler();  // uC/OS-III naming convention
void __attribute__((weak)) OS_CPU_SysTickHandler();

void __attribute__((weak)) DMA0_IRQHandler();
void __attribute__((weak)) DMA1_IRQHandler();
void __attribute__((weak)) DMA2_IRQHandler();
void __attribute__((weak)) DMA3_IRQHandler();
void __attribute__((weak)) DMA4_IRQHandler();
void __attribute__((weak)) DMA5_IRQHandler();
void __attribute__((weak)) DMA6_IRQHandler();
void __attribute__((weak)) DMA7_IRQHandler();
void __attribute__((weak)) DMA8_IRQHandler();
void __attribute__((weak)) DMA9_IRQHandler();
void __attribute__((weak)) DMA10_IRQHandler();
void __attribute__((weak)) DMA11_IRQHandler();
void __attribute__((weak)) DMA12_IRQHandler();
void __attribute__((weak)) DMA13_IRQHandler();
void __attribute__((weak)) DMA14_IRQHandler();
void __attribute__((weak)) DMA15_IRQHandler();
void __attribute__((weak)) DMA_Error_IRQHandler();
void __attribute__((weak)) MCM_IRQHandler();
void __attribute__((weak)) FTF_IRQHandler();
void __attribute__((weak)) Read_Collision_IRQHandler();
void __attribute__((weak)) LVD_LVW_IRQHandler();
void __attribute__((weak)) LLWU_IRQHandler();
void __attribute__((weak)) WDOG_EWM_IRQHandler();
void __attribute__((weak)) RNG_IRQHandler();
void __attribute__((weak)) I2C0_IRQHandler();
void __attribute__((weak)) I2C1_IRQHandler();
void __attribute__((weak)) SPI0_IRQHandler();
void __attribute__((weak)) SPI1_IRQHandler();
void __attribute__((weak)) I2S0_Tx_IRQHandler();
void __attribute__((weak)) I2S0_Rx_IRQHandler();
void __attribute__((weak)) LPUART0_IRQHandler();
void __attribute__((weak)) UART0_RX_TX_IRQHandler();
void __attribute__((weak)) UART0_ERR_IRQHandler();
void __attribute__((weak)) UART1_RX_TX_IRQHandler();
void __attribute__((weak)) UART1_ERR_IRQHandler();
void __attribute__((weak)) UART2_RX_TX_IRQHandler();
void __attribute__((weak)) UART2_ERR_IRQHandler();
void __attribute__((weak)) Reserved53_IRQHandler();
void __attribute__((weak)) Reserved54_IRQHandler();
void __attribute__((weak)) ADC0_IRQHandler();
void __attribute__((weak)) CMP0_IRQHandler();
void __attribute__((weak)) CMP1_IRQHandler();
void __attribute__((weak)) FTM0_IRQHandler();
void __attribute__((weak)) FTM1_IRQHandler();
void __attribute__((weak)) FTM2_IRQHandler();
void __attribute__((weak)) Reserved61_IRQHandler();
void __attribute__((weak)) RTC_IRQHandler();
void __attribute__((weak)) RTC_Seconds_IRQHandler();
void __attribute__((weak)) PIT0_IRQHandler();
void __attribute__((weak)) PIT1_IRQHandler();
void __attribute__((weak)) PIT2_IRQHandler();
void __attribute__((weak)) PIT3_IRQHandler();
void __attribute__((weak)) PDB0_IRQHandler();
void __attribute__((weak)) USB0_IRQHandler();
void __attribute__((weak)) Reserved70_IRQHandler();
void __attribute__((weak)) Reserved71_IRQHandler();
void __attribute__((weak)) DAC0_IRQHandler();
void __attribute__((weak)) MCG_IRQHandler();
void __attribute__((weak)) LPTMR0_IRQHandler();
void __attribute__((weak)) PORTA_IRQHandler();
void __attribute__((weak)) PORTB_IRQHandler();
void __attribute__((weak)) PORTC_IRQHandler();
void __attribute__((weak)) PORTD_IRQHandler();
void __attribute__((weak)) PORTE_IRQHandler();
void __attribute__((weak)) SWI_IRQHandler();
void __attribute__((weak)) Reserved81_IRQHandler();
void __attribute__((weak)) Reserved82_IRQHandler();
void __attribute__((weak)) Reserved83_IRQHandler();
void __attribute__((weak)) Reserved84_IRQHandler();
void __attribute__((weak)) Reserved85_IRQHandler();
void __attribute__((weak)) Reserved86_IRQHandler();
void __attribute__((weak)) FTM3_IRQHandler();
void __attribute__((weak)) DAC1_IRQHandler();
void __attribute__((weak)) ADC1_IRQHandler();

//Stack top, defined in the linker script
extern char _stack_top asm("_stack_top");

//Interrupt vectors, must be placed @ address 0x00000000
//The extern declaration is required otherwise g++ optimizes it out
extern void (* const __Vectors[])();
void (* const __Vectors[])() __attribute__ ((section(".isr_vector"))) =
{
    (void (*)())(&_stack_top),
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
    OS_CPU_PendSVHandler,
    OS_CPU_SysTickHandler,

    DMA0_IRQHandler,            /* DMA Channel 0 Transfer Complete*/
    DMA1_IRQHandler,            /* DMA Channel 1 Transfer Complete*/
    DMA2_IRQHandler,            /* DMA Channel 2 Transfer Complete*/
    DMA3_IRQHandler,            /* DMA Channel 3 Transfer Complete*/
    DMA4_IRQHandler,            /* DMA Channel 4 Transfer Complete*/
    DMA5_IRQHandler,            /* DMA Channel 5 Transfer Complete*/
    DMA6_IRQHandler,            /* DMA Channel 6 Transfer Complete*/
    DMA7_IRQHandler,            /* DMA Channel 7 Transfer Complete*/
    DMA8_IRQHandler,            /* DMA Channel 8 Transfer Complete*/
    DMA9_IRQHandler,            /* DMA Channel 9 Transfer Complete*/
    DMA10_IRQHandler,           /* DMA Channel 10 Transfer Complete*/
    DMA11_IRQHandler,           /* DMA Channel 11 Transfer Complete*/
    DMA12_IRQHandler,           /* DMA Channel 12 Transfer Complete*/
    DMA13_IRQHandler,           /* DMA Channel 13 Transfer Complete*/
    DMA14_IRQHandler,           /* DMA Channel 14 Transfer Complete*/
    DMA15_IRQHandler,           /* DMA Channel 15 Transfer Complete*/
    DMA_Error_IRQHandler,       /* DMA Error Interrupt*/
    MCM_IRQHandler,             /* Normal Interrupt*/
    FTF_IRQHandler,             /* FTFA Command complete interrupt*/
    Read_Collision_IRQHandler,  /* Read Collision Interrupt*/
    LVD_LVW_IRQHandler,         /* Low Voltage Detect, Low Voltage Warning*/
    LLWU_IRQHandler,            /* Low Leakage Wakeup Unit*/
    WDOG_EWM_IRQHandler,        /* WDOG Interrupt*/
    RNG_IRQHandler,             /* RNG Interrupt*/
    I2C0_IRQHandler,            /* I2C0 interrupt*/
    I2C1_IRQHandler,            /* I2C1 interrupt*/
    SPI0_IRQHandler,            /* SPI0 Interrupt*/
    SPI1_IRQHandler,            /* SPI1 Interrupt*/
    I2S0_Tx_IRQHandler,         /* I2S0 transmit interrupt*/
    I2S0_Rx_IRQHandler,         /* I2S0 receive interrupt*/
    LPUART0_IRQHandler,         /* LPUART0 status/error interrupt*/
    UART0_RX_TX_IRQHandler,     /* UART0 Receive/Transmit interrupt*/
    UART0_ERR_IRQHandler,       /* UART0 Error interrupt*/
    UART1_RX_TX_IRQHandler,     /* UART1 Receive/Transmit interrupt*/
    UART1_ERR_IRQHandler,       /* UART1 Error interrupt*/
    UART2_RX_TX_IRQHandler,     /* UART2 Receive/Transmit interrupt*/
    UART2_ERR_IRQHandler,       /* UART2 Error interrupt*/
    Reserved53_IRQHandler,      /* Reserved interrupt 53*/
    Reserved54_IRQHandler,      /* Reserved interrupt 54*/
    ADC0_IRQHandler,            /* ADC0 interrupt*/
    CMP0_IRQHandler,            /* CMP0 interrupt*/
    CMP1_IRQHandler,            /* CMP1 interrupt*/
    FTM0_IRQHandler,            /* FTM0 fault, overflow and channels interrupt*/
    FTM1_IRQHandler,            /* FTM1 fault, overflow and channels interrupt*/
    FTM2_IRQHandler,            /* FTM2 fault, overflow and channels interrupt*/
    Reserved61_IRQHandler,      /* Reserved interrupt 61*/
    RTC_IRQHandler,             /* RTC interrupt*/
    RTC_Seconds_IRQHandler,     /* RTC seconds interrupt*/
    PIT0_IRQHandler,            /* PIT timer channel 0 interrupt*/
    PIT1_IRQHandler,            /* PIT timer channel 1 interrupt*/
    PIT2_IRQHandler,            /* PIT timer channel 2 interrupt*/
    PIT3_IRQHandler,            /* PIT timer channel 3 interrupt*/
    PDB0_IRQHandler,            /* PDB0 Interrupt*/
    USB0_IRQHandler,            /* USB0 interrupt*/
    Reserved70_IRQHandler,      /* Reserved interrupt 70*/
    Reserved71_IRQHandler,      /* Reserved interrupt 71*/
    DAC0_IRQHandler,            /* DAC0 interrupt*/
    MCG_IRQHandler,             /* MCG Interrupt*/
    LPTMR0_IRQHandler,          /* LPTimer interrupt*/
    PORTA_IRQHandler,           /* Port A interrupt*/
    PORTB_IRQHandler,           /* Port B interrupt*/
    PORTC_IRQHandler,           /* Port C interrupt*/
    PORTD_IRQHandler,           /* Port D interrupt*/
    PORTE_IRQHandler,           /* Port E interrupt*/
    SWI_IRQHandler,             /* Software interrupt*/
    Reserved81_IRQHandler,      /* Reserved interrupt 81*/
    Reserved82_IRQHandler,      /* Reserved interrupt 82*/
    Reserved83_IRQHandler,      /* Reserved interrupt 83*/
    Reserved84_IRQHandler,      /* Reserved interrupt 84*/
    Reserved85_IRQHandler,      /* Reserved interrupt 85*/
    Reserved86_IRQHandler,      /* Reserved interrupt 86*/
    FTM3_IRQHandler,            /* FTM3 fault, overflow and channels interrupt*/
    DAC1_IRQHandler,            /* DAC1 interrupt*/
    ADC1_IRQHandler             /* ADC1 interrupt*/
};

#pragma weak NMI_Handler= Default_Handler
#pragma weak HardFault_Handler= Default_Handler
#pragma weak MemManage_Handler= Default_Handler
#pragma weak BusFault_Handler= Default_Handler
#pragma weak UsageFault_Handler= Default_Handler
#pragma weak SVC_Handler= Default_Handler
#pragma weak DebugMon_Handler= Default_Handler
#pragma weak PendSV_Handler= Default_Handler
#pragma weak SysTick_Handler= Default_Handler
#pragma weak WWDG_IRQHandler= Default_Handler

#pragma weak DMA0_IRQHandler = Default_Handler
#pragma weak DMA1_IRQHandler = Default_Handler
#pragma weak DMA2_IRQHandler = Default_Handler
#pragma weak DMA3_IRQHandler = Default_Handler
#pragma weak DMA4_IRQHandler = Default_Handler
#pragma weak DMA5_IRQHandler = Default_Handler
#pragma weak DMA6_IRQHandler = Default_Handler
#pragma weak DMA7_IRQHandler = Default_Handler
#pragma weak DMA8_IRQHandler = Default_Handler
#pragma weak DMA9_IRQHandler = Default_Handler
#pragma weak DMA10_IRQHandler = Default_Handler
#pragma weak DMA11_IRQHandler = Default_Handler
#pragma weak DMA12_IRQHandler = Default_Handler
#pragma weak DMA13_IRQHandler = Default_Handler
#pragma weak DMA14_IRQHandler = Default_Handler
#pragma weak DMA15_IRQHandler = Default_Handler
#pragma weak DMA_Error_IRQHandler = Default_Handler
#pragma weak MCM_IRQHandler = Default_Handler
#pragma weak FTF_IRQHandler = Default_Handler
#pragma weak Read_Collision_IRQHandler = Default_Handler
#pragma weak LVD_LVW_IRQHandler = Default_Handler
#pragma weak LLWU_IRQHandler = Default_Handler
#pragma weak WDOG_EWM_IRQHandler = Default_Handler
#pragma weak RNG_IRQHandler = Default_Handler
#pragma weak I2C0_IRQHandler = Default_Handler
#pragma weak I2C1_IRQHandler = Default_Handler
#pragma weak SPI0_IRQHandler = Default_Handler
#pragma weak SPI1_IRQHandler = Default_Handler
#pragma weak I2S0_Tx_IRQHandler = Default_Handler
#pragma weak I2S0_Rx_IRQHandler = Default_Handler
#pragma weak LPUART0_IRQHandler = Default_Handler
#pragma weak UART0_RX_TX_IRQHandler = Default_Handler
#pragma weak UART0_ERR_IRQHandler = Default_Handler
#pragma weak UART1_RX_TX_IRQHandler = Default_Handler
#pragma weak UART1_ERR_IRQHandler = Default_Handler
#pragma weak UART2_RX_TX_IRQHandler = Default_Handler
#pragma weak UART2_ERR_IRQHandler = Default_Handler
#pragma weak Reserved53_IRQHandler = Default_Handler
#pragma weak Reserved54_IRQHandler = Default_Handler
#pragma weak ADC0_IRQHandler = Default_Handler
#pragma weak CMP0_IRQHandler = Default_Handler
#pragma weak CMP1_IRQHandler = Default_Handler
#pragma weak FTM0_IRQHandler = Default_Handler
#pragma weak FTM1_IRQHandler = Default_Handler
#pragma weak FTM2_IRQHandler = Default_Handler
#pragma weak Reserved61_IRQHandler = Default_Handler
#pragma weak RTC_IRQHandler = Default_Handler
#pragma weak RTC_Seconds_IRQHandler = Default_Handler
#pragma weak PIT0_IRQHandler = Default_Handler
#pragma weak PIT1_IRQHandler = Default_Handler
#pragma weak PIT2_IRQHandler = Default_Handler
#pragma weak PIT3_IRQHandler = Default_Handler
#pragma weak PDB0_IRQHandler = Default_Handler
#pragma weak USB0_IRQHandler = Default_Handler
#pragma weak Reserved70_IRQHandler = Default_Handler
#pragma weak Reserved71_IRQHandler = Default_Handler
#pragma weak DAC0_IRQHandler = Default_Handler
#pragma weak MCG_IRQHandler = Default_Handler
#pragma weak LPTMR0_IRQHandler = Default_Handler
#pragma weak PORTA_IRQHandler = Default_Handler
#pragma weak PORTB_IRQHandler = Default_Handler
#pragma weak PORTC_IRQHandler = Default_Handler
#pragma weak PORTD_IRQHandler = Default_Handler
#pragma weak PORTE_IRQHandler = Default_Handler
#pragma weak SWI_IRQHandler = Default_Handler
#pragma weak Reserved81_IRQHandler = Default_Handler
#pragma weak Reserved82_IRQHandler = Default_Handler
#pragma weak Reserved83_IRQHandler = Default_Handler
#pragma weak Reserved84_IRQHandler = Default_Handler
#pragma weak Reserved85_IRQHandler = Default_Handler
#pragma weak Reserved86_IRQHandler = Default_Handler
#pragma weak FTM3_IRQHandler = Default_Handler
#pragma weak DAC1_IRQHandler = Default_Handler
#pragma weak ADC1_IRQHandler = Default_Handler
