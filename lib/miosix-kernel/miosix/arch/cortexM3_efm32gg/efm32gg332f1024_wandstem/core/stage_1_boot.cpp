
#include "interfaces/arch_registers.h"
#include "interfaces/bsp.h"
#include "core/interrupts.h" //For the unexpected interrupt call
#include "kernel/stage_2_boot.h"
#include <string.h>

/*
 * EFM32 C++ startup, supports interrupt handlers in C++ without extern "C"
 * NOTE: for EFM32GG devices ONLY.
 * Developed by Terraneo Federico to boot Miosix,
 * based on silicon laboratories startup code.
 */

/**
 * Called by Reset_Handler, performs initialization and calls main.
 * Never returns.
 */
void __attribute__((noreturn)) program_startup()
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
    memcpy(data,etext,edata-data);
    memset(bss_start,0,bss_end-bss_start);

    //Move on to stage 2
    _init();

    //If main returns, reboot
    NVIC_SystemReset();
    for(;;) ;
}

/**
 * Reset handler, called by hardware immediately after reset
 */
void __attribute__((__interrupt__, noreturn)) Reset_Handler()
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
void __attribute__((weak)) SysTick_Handler();

//Interrupt handlers
void __attribute__((weak)) DMA_IRQHandler();
void __attribute__((weak)) GPIO_EVEN_IRQHandler();
void __attribute__((weak)) TIMER0_IRQHandler();
void __attribute__((weak)) USART0_RX_IRQHandler();
void __attribute__((weak)) USART0_TX_IRQHandler();
void __attribute__((weak)) USB_IRQHandler();
void __attribute__((weak)) ACMP0_IRQHandler();
void __attribute__((weak)) ADC0_IRQHandler();
void __attribute__((weak)) DAC0_IRQHandler();
void __attribute__((weak)) I2C0_IRQHandler();
void __attribute__((weak)) I2C1_IRQHandler();
void __attribute__((weak)) GPIO_ODD_IRQHandler();
void __attribute__((weak)) TIMER1_IRQHandler();
void __attribute__((weak)) TIMER2_IRQHandler();
void __attribute__((weak)) TIMER3_IRQHandler();
void __attribute__((weak)) USART1_RX_IRQHandler();
void __attribute__((weak)) USART1_TX_IRQHandler();
void __attribute__((weak)) LESENSE_IRQHandler();
void __attribute__((weak)) USART2_RX_IRQHandler();
void __attribute__((weak)) USART2_TX_IRQHandler();
void __attribute__((weak)) UART0_RX_IRQHandler();
void __attribute__((weak)) UART0_TX_IRQHandler();
void __attribute__((weak)) UART1_RX_IRQHandler();
void __attribute__((weak)) UART1_TX_IRQHandler();
void __attribute__((weak)) LEUART0_IRQHandler();
void __attribute__((weak)) LEUART1_IRQHandler();
void __attribute__((weak)) LETIMER0_IRQHandler();
void __attribute__((weak)) PCNT0_IRQHandler();
void __attribute__((weak)) PCNT1_IRQHandler();
void __attribute__((weak)) PCNT2_IRQHandler();
void __attribute__((weak)) RTC_IRQHandler();
void __attribute__((weak)) BURTC_IRQHandler();
void __attribute__((weak)) CMU_IRQHandler();
void __attribute__((weak)) VCMP_IRQHandler();
void __attribute__((weak)) LCD_IRQHandler();
void __attribute__((weak)) MSC_IRQHandler();
void __attribute__((weak)) AES_IRQHandler();
void __attribute__((weak)) EBI_IRQHandler();
void __attribute__((weak)) EMU_IRQHandler();

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
    DMA_IRQHandler,
    GPIO_EVEN_IRQHandler,
    TIMER0_IRQHandler,
    USART0_RX_IRQHandler,
    USART0_TX_IRQHandler,
    USB_IRQHandler,
    ACMP0_IRQHandler,
    ADC0_IRQHandler,
    DAC0_IRQHandler,
    I2C0_IRQHandler,
    I2C1_IRQHandler,
    GPIO_ODD_IRQHandler,
    TIMER1_IRQHandler,
    TIMER2_IRQHandler,
    TIMER3_IRQHandler,
    USART1_RX_IRQHandler,
    USART1_TX_IRQHandler,
    LESENSE_IRQHandler,
    USART2_RX_IRQHandler,
    USART2_TX_IRQHandler,
    UART0_RX_IRQHandler,
    UART0_TX_IRQHandler,
    UART1_RX_IRQHandler,
    UART1_TX_IRQHandler,
    LEUART0_IRQHandler,
    LEUART1_IRQHandler,
    LETIMER0_IRQHandler,
    PCNT0_IRQHandler,
    PCNT1_IRQHandler,
    PCNT2_IRQHandler,
    RTC_IRQHandler,
    BURTC_IRQHandler,
    CMU_IRQHandler,
    VCMP_IRQHandler,
    LCD_IRQHandler,
    MSC_IRQHandler,
    AES_IRQHandler,
    EBI_IRQHandler,
    EMU_IRQHandler
};

#pragma weak DMA_IRQHandler = Default_Handler
#pragma weak GPIO_EVEN_IRQHandler = Default_Handler
#pragma weak TIMER0_IRQHandler = Default_Handler
#pragma weak USART0_RX_IRQHandler = Default_Handler
#pragma weak USART0_TX_IRQHandler = Default_Handler
#pragma weak USB_IRQHandler = Default_Handler
#pragma weak ACMP0_IRQHandler = Default_Handler
#pragma weak ADC0_IRQHandler = Default_Handler
#pragma weak DAC0_IRQHandler = Default_Handler
#pragma weak I2C0_IRQHandler = Default_Handler
#pragma weak I2C1_IRQHandler = Default_Handler
#pragma weak GPIO_ODD_IRQHandler = Default_Handler
#pragma weak TIMER1_IRQHandler = Default_Handler
#pragma weak TIMER2_IRQHandler = Default_Handler
#pragma weak TIMER3_IRQHandler = Default_Handler
#pragma weak USART1_RX_IRQHandler = Default_Handler
#pragma weak USART1_TX_IRQHandler = Default_Handler
#pragma weak LESENSE_IRQHandler = Default_Handler
#pragma weak USART2_RX_IRQHandler = Default_Handler
#pragma weak USART2_TX_IRQHandler = Default_Handler
#pragma weak UART0_RX_IRQHandler = Default_Handler
#pragma weak UART0_TX_IRQHandler = Default_Handler
#pragma weak UART1_RX_IRQHandler = Default_Handler
#pragma weak UART1_TX_IRQHandler = Default_Handler
#pragma weak LEUART0_IRQHandler = Default_Handler
#pragma weak LEUART1_IRQHandler = Default_Handler
#pragma weak LETIMER0_IRQHandler = Default_Handler
#pragma weak PCNT0_IRQHandler = Default_Handler
#pragma weak PCNT1_IRQHandler = Default_Handler
#pragma weak PCNT2_IRQHandler = Default_Handler
#pragma weak RTC_IRQHandler = Default_Handler
#pragma weak BURTC_IRQHandler = Default_Handler
#pragma weak CMU_IRQHandler = Default_Handler
#pragma weak VCMP_IRQHandler = Default_Handler
#pragma weak LCD_IRQHandler = Default_Handler
#pragma weak MSC_IRQHandler = Default_Handler
#pragma weak AES_IRQHandler = Default_Handler
#pragma weak EBI_IRQHandler = Default_Handler
#pragma weak EMU_IRQHandler = Default_Handler
#pragma weak SysTick_Handler = Default_Handler