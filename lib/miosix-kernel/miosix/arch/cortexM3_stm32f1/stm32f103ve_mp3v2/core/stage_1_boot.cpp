
#include "interfaces/arch_registers.h"
#include "core/interrupts.h" //For the unexpected interrupt call
#include "kernel/stage_2_boot.h"
#include <string.h>

/*
 * startup.cpp
 * STM32 C++ startup.
 * Supports interrupt handlers in C++ without extern "C"
 * Developed by Terraneo Federico, based on ST startup code.
 * Additionally modified to boot Miosix.
 */

/**
 * Called by Reset_Handler, performs initialization and calls main.
 * Never returns.
 */
void program_startup() __attribute__((noreturn));
void program_startup()
{
    //Cortex M3 core appears to get out of reset with interrupts already enabled
    __disable_irq();

	//Not calling SystemInit() here since the bootloader already does that for us,
	//and calling it twice seems to mess up things
    //SystemInit();

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
    #ifdef __CODE_IN_XRAM
    /**
     * Before calling the initalization code, set the stack pointer to the
     * required value. At first it might seem redundant setting the stack
     * pointer since the hardware should take care of this, but there is a
     * corner case in which it is not set properly:
     * 1) A debugger like openocd is used to run the code
     * 2) Debugged code is run from external RAM
     * In this case, in FLASH starting from 0x00000000 there is a bootloader
     * that forwards interrupt vectors to their address in external RAM
     * at address 0x68000000. The bootloader can also be used to load the code
     * but this is uninteresting here. The problem is that the bootloader uses
     * the internal RAM for its stack, so @ 0x00000000 there is the value
     * 0x20000000 (top of internal RAM). When a debugger like openocd loads the
     * debuged code in external RAM starting from 0x68000000 it mimics the
     * harware behaviour of setting the stack pointer. But it loads the value at
     * 0x00000000, not the value at 0x68000000. Therefore the stack pointer
     * is set to the stak used by the bootloader (top of INTERNAL RAM) instead
     * of the stack of the debugged code (top of EXTERNAL RAM).
     * Since this quirk happens only when running code from external RAM, the
     * fix is enclosed in an #ifdef __CODE_IN_XRAM
     */
    asm volatile("ldr sp, =_main_stack_top\n\t");
    #endif //__CODE_IN_XRAM

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

/**
 * ?
 */
extern "C" void SystemInit_ExtMemCtl_Dummy() 
{
	//Do nothing
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
void __attribute__((weak)) WWDG_IRQHandler();
void __attribute__((weak)) PVD_IRQHandler();
void __attribute__((weak)) TAMPER_IRQHandler();
void __attribute__((weak)) RTC_IRQHandler();
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
void __attribute__((weak)) USB_HP_CAN1_TX_IRQHandler();
void __attribute__((weak)) USB_LP_CAN1_RX0_IRQHandler();
void __attribute__((weak)) CAN1_RX1_IRQHandler();
void __attribute__((weak)) CAN1_SCE_IRQHandler();
void __attribute__((weak)) EXTI9_5_IRQHandler();
void __attribute__((weak)) TIM1_BRK_IRQHandler();
void __attribute__((weak)) TIM1_UP_IRQHandler();
void __attribute__((weak)) TIM1_TRG_COM_IRQHandler();
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
void __attribute__((weak)) RTCAlarm_IRQHandler();
void __attribute__((weak)) USBWakeUp_IRQHandler();
void __attribute__((weak)) TIM8_BRK_IRQHandler();
void __attribute__((weak)) TIM8_UP_IRQHandler();
void __attribute__((weak)) TIM8_TRG_COM_IRQHandler();
void __attribute__((weak)) TIM8_CC_IRQHandler();
void __attribute__((weak)) ADC3_IRQHandler();
void __attribute__((weak)) FSMC_IRQHandler();
void __attribute__((weak)) SDIO_IRQHandler();
void __attribute__((weak)) TIM5_IRQHandler();
void __attribute__((weak)) SPI3_IRQHandler();
void __attribute__((weak)) UART4_IRQHandler();
void __attribute__((weak)) UART5_IRQHandler();
void __attribute__((weak)) TIM6_IRQHandler();
void __attribute__((weak)) TIM7_IRQHandler();
void __attribute__((weak)) DMA2_Channel1_IRQHandler();
void __attribute__((weak)) DMA2_Channel2_IRQHandler();
void __attribute__((weak)) DMA2_Channel3_IRQHandler();
void __attribute__((weak)) DMA2_Channel4_5_IRQHandler();
void __attribute__((weak)) SystemInit_ExtMemCtl(); 

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
    WWDG_IRQHandler,            /* Window Watchdog */
    PVD_IRQHandler,             /* PVD through EXTI Line detect */
    TAMPER_IRQHandler,          /* Tamper */
    RTC_IRQHandler,             /* RTC */
    FLASH_IRQHandler,           /* Flash */
    RCC_IRQHandler,             /* RCC */
    EXTI0_IRQHandler,           /* EXTI Line 0 */
    EXTI1_IRQHandler,           /* EXTI Line 1 */
    EXTI2_IRQHandler,           /* EXTI Line 2 */
    EXTI3_IRQHandler,           /* EXTI Line 3 */
    EXTI4_IRQHandler,           /* EXTI Line 4 */
    DMA1_Channel1_IRQHandler,   /* DMA1 Channel 1 */
    DMA1_Channel2_IRQHandler,   /* DMA1 Channel 2 */
    DMA1_Channel3_IRQHandler,   /* DMA1 Channel 3 */
    DMA1_Channel4_IRQHandler,   /* DMA1 Channel 4 */
    DMA1_Channel5_IRQHandler,   /* DMA1 Channel 5 */
    DMA1_Channel6_IRQHandler,   /* DMA1 Channel 6 */
    DMA1_Channel7_IRQHandler,   /* DMA1 Channel 7 */
    ADC1_2_IRQHandler,          /* ADC1 & ADC2 */
    USB_HP_CAN1_TX_IRQHandler,  /* USB High Priority or CAN1 TX */
    USB_LP_CAN1_RX0_IRQHandler, /* USB Low  Priority or CAN1 RX0 */
    CAN1_RX1_IRQHandler,        /* CAN1 RX1 */
    CAN1_SCE_IRQHandler,        /* CAN1 SCE */
    EXTI9_5_IRQHandler,         /* EXTI Line 9..5 */
    TIM1_BRK_IRQHandler,        /* TIM1 Break */
    TIM1_UP_IRQHandler,         /* TIM1 Update */
    TIM1_TRG_COM_IRQHandler,    /* TIM1 Trigger and Commutation */
    TIM1_CC_IRQHandler,         /* TIM1 Capture Compare */
    TIM2_IRQHandler,            /* TIM2 */
    TIM3_IRQHandler,            /* TIM3 */
    TIM4_IRQHandler,            /* TIM4 */
    I2C1_EV_IRQHandler,         /* I2C1 Event */
    I2C1_ER_IRQHandler,         /* I2C1 Error */
    I2C2_EV_IRQHandler,         /* I2C2 Event */
    I2C2_ER_IRQHandler,         /* I2C2 Error */
    SPI1_IRQHandler,            /* SPI1 */
    SPI2_IRQHandler,            /* SPI2 */
    USART1_IRQHandler,          /* USART1 */
    USART2_IRQHandler,          /* USART2 */
    USART3_IRQHandler,          /* USART3 */
    EXTI15_10_IRQHandler,       /* EXTI Line 15..10 */
    RTCAlarm_IRQHandler,        /* RTC Alarm through EXTI Line */
    USBWakeUp_IRQHandler,       /* USB Wakeup from suspend */  
    TIM8_BRK_IRQHandler,        /* TIM8 Break */
    TIM8_UP_IRQHandler,         /* TIM8 Update */
    TIM8_TRG_COM_IRQHandler,    /* TIM8 Trigger and Commutation */
    TIM8_CC_IRQHandler,         /* TIM8 Capture Compare */
    ADC3_IRQHandler,            /* ADC3 */
    FSMC_IRQHandler,            /* FSMC */
    SDIO_IRQHandler,            /* SDIO */
    TIM5_IRQHandler,            /* TIM5 */
    SPI3_IRQHandler,            /* SPI3 */
    UART4_IRQHandler,           /* UART4 */ 
    UART5_IRQHandler,           /* UART5 */
    TIM6_IRQHandler,            /* TIM6 */ 
    TIM7_IRQHandler,            /* TIM7 */
    DMA2_Channel1_IRQHandler,   /* DMA2 Channel 1 */ 
    DMA2_Channel2_IRQHandler,   /* DMA2 Channel 2 */ 
    DMA2_Channel3_IRQHandler,   /* DMA2 Channel 3 */ 
    DMA2_Channel4_5_IRQHandler, /* DMA2 Channel 4 and Channel 5 */ 
    0,0,0,0,0,0,0,0,            /* @0x130 */ 
    0,0,0,0,0,0,0,0,            /* @0x150 */
    0,0,0,0,0,0,0,0,            /* @0x170 */
    0,0,0,0,0,0,0,0,            /* @0x190 */
    0,0,0,0,0,0,0,0,            /* @0x1B0 */
    0,0,0,0,                    /* @0x1D0 */
    reinterpret_cast<void (*)()>(0xF1E0F85F)
                                /* @0x1E0. This is for boot in RAM mode for
                                   STM32F10x High Density devices.*/
};

#pragma weak SysTick_Handler = Default_Handler
#pragma weak WWDG_IRQHandler = Default_Handler
#pragma weak PVD_IRQHandler = Default_Handler
#pragma weak TAMPER_IRQHandler = Default_Handler
#pragma weak RTC_IRQHandler = Default_Handler
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
#pragma weak USB_HP_CAN1_TX_IRQHandler = Default_Handler
#pragma weak USB_LP_CAN1_RX0_IRQHandler = Default_Handler
#pragma weak CAN1_RX1_IRQHandler = Default_Handler
#pragma weak CAN1_SCE_IRQHandler = Default_Handler
#pragma weak EXTI9_5_IRQHandler = Default_Handler
#pragma weak TIM1_BRK_IRQHandler = Default_Handler
#pragma weak TIM1_UP_IRQHandler = Default_Handler
#pragma weak TIM1_TRG_COM_IRQHandler = Default_Handler
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
#pragma weak RTCAlarm_IRQHandler = Default_Handler
#pragma weak USBWakeUp_IRQHandler = Default_Handler
#pragma weak TIM8_BRK_IRQHandler = Default_Handler
#pragma weak TIM8_UP_IRQHandler = Default_Handler
#pragma weak TIM8_TRG_COM_IRQHandler = Default_Handler
#pragma weak TIM8_CC_IRQHandler = Default_Handler
#pragma weak ADC3_IRQHandler = Default_Handler
#pragma weak FSMC_IRQHandler = Default_Handler
#pragma weak SDIO_IRQHandler = Default_Handler
#pragma weak TIM5_IRQHandler = Default_Handler
#pragma weak SPI3_IRQHandler = Default_Handler
#pragma weak UART4_IRQHandler = Default_Handler
#pragma weak UART5_IRQHandler = Default_Handler
#pragma weak TIM6_IRQHandler = Default_Handler
#pragma weak TIM7_IRQHandler = Default_Handler
#pragma weak DMA2_Channel1_IRQHandler = Default_Handler
#pragma weak DMA2_Channel2_IRQHandler = Default_Handler
#pragma weak DMA2_Channel3_IRQHandler = Default_Handler
#pragma weak DMA2_Channel4_5_IRQHandler = Default_Handler
#pragma weak SystemInit_ExtMemCtl = SystemInit_ExtMemCtl_Dummy
