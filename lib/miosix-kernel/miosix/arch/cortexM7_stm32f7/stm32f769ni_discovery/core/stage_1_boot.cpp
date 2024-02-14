#include <string.h>

#include "core/cache_cortexMx.h"
#include "core/interrupts.h"  //For the unexpected interrupt call
#include "core/interrupts_cortexMx.h"
#include "interfaces/arch_registers.h"
#include "interfaces/bsp.h"
#include "kernel/stage_2_boot.h"

/*
 * startup.cpp
 * STM32 C++ startup.
 * NOTE: for stm32f769 devices ONLY.
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
    // Cortex M7 core appears to get out of reset with interrupts already
    // enabled
    __disable_irq();

    miosix::IRQconfigureCache((const unsigned int*)0xc0000000, 16 * 1024 * 1024);

    // These are defined in the linker script
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

    // Move on to stage 2
    _init();

    // If main returns, reboot
    NVIC_SystemReset();
    for(;;) ;
}

/**
 * Reset handler, called by hardware immediately after reset
 */
void Reset_Handler() __attribute__((__interrupt__, noreturn));
void Reset_Handler()
{
    /**
     * SystemInit() is called *before* initializing .data and zeroing .bss
     * Despite all startup files provided by ST do the opposite, there are three
     * good reasons to do so:
     * 1. First, the CMSIS specifications say that SystemInit() must not access
     *    global variables, so it is actually possible to call it before
     * 2. Second, when running Miosix with the xram linker scripts .data and
     *    .bss are placed in the external RAM, so we *must* call SystemInit(),
     *    which enables xram, before touching .data and .bss
     * 3. Third, this is a performance improvement since the loops that
     *    initialize .data and zeros .bss now run with the CPU at full speed
     *    instead of 8MHz
     */
    SystemInit();

    /**
     * ST does not provide code to initialize the SDRAM at boot.
     * Put after SystemInit() as SDRAM is timing-sensitive and needs the full
     * clock speed.
     */
    #ifdef __ENABLE_XRAM
    miosix::configureSdram();
    #endif  //__ENABLE_XRAM

    /*
     * Load into the program stack pointer the heap end address and switch from
     * the msp to sps.
     * This is required for booting Miosix, a small portion of the top of the
     * heap area will be used as stack until the first thread starts. After,
     * this stack will be abandoned and the process stack will point to the
     * current thread's stack.
     */
    asm volatile(
        "ldr r0,  =_heap_end          \n\t"
        "msr psp, r0                  \n\t"
        "movw r0, #2                  \n\n"  // Set the control register to use
        "msr control, r0              \n\t"  // the process stack
        "isb                          \n\t":::"r0");

    program_startup();
}

/**
 * All unused interrupts call this function.
 */
extern "C" void Default_Handler() { unexpectedInterrupt(); }

// System handlers
void /*__attribute__((weak))*/ Reset_Handler();      // These interrupts are not
void /*__attribute__((weak))*/ NMI_Handler();        // weak because they are
void /*__attribute__((weak))*/ HardFault_Handler();  // surely defined by Miosix
void /*__attribute__((weak))*/ MemManage_Handler();
void /*__attribute__((weak))*/ BusFault_Handler();
void /*__attribute__((weak))*/ UsageFault_Handler();
void /*__attribute__((weak))*/ SVC_Handler();
void /*__attribute__((weak))*/ DebugMon_Handler();
void /*__attribute__((weak))*/ PendSV_Handler();
void __attribute__((weak)) SysTick_Handler();

// Interrupt handlers
void __attribute__((weak)) WWDG_IRQHandler();
void __attribute__((weak)) PVD_IRQHandler();
void __attribute__((weak)) TAMP_STAMP_IRQHandler();
void __attribute__((weak)) RTC_WKUP_IRQHandler();
void __attribute__((weak)) FLASH_IRQHandler();
void __attribute__((weak)) RCC_IRQHandler();
void __attribute__((weak)) EXTI0_IRQHandler();
void __attribute__((weak)) EXTI1_IRQHandler();
void __attribute__((weak)) EXTI2_IRQHandler();
void __attribute__((weak)) EXTI3_IRQHandler();
void __attribute__((weak)) EXTI4_IRQHandler();
void __attribute__((weak)) DMA1_Stream0_IRQHandler();
void __attribute__((weak)) DMA1_Stream1_IRQHandler();
void __attribute__((weak)) DMA1_Stream2_IRQHandler();
void __attribute__((weak)) DMA1_Stream3_IRQHandler();
void __attribute__((weak)) DMA1_Stream4_IRQHandler();
void __attribute__((weak)) DMA1_Stream5_IRQHandler();
void __attribute__((weak)) DMA1_Stream6_IRQHandler();
void __attribute__((weak)) ADC_IRQHandler();
void __attribute__((weak)) CAN1_TX_IRQHandler();
void __attribute__((weak)) CAN1_RX0_IRQHandler();
void __attribute__((weak)) CAN1_RX1_IRQHandler();
void __attribute__((weak)) CAN1_SCE_IRQHandler();
void __attribute__((weak)) EXTI9_5_IRQHandler();
void __attribute__((weak)) TIM1_BRK_TIM9_IRQHandler();
void __attribute__((weak)) TIM1_UP_TIM10_IRQHandler();
void __attribute__((weak)) TIM1_TRG_COM_TIM11_IRQHandler();
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
void __attribute__((weak)) OTG_FS_WKUP_IRQHandler();
void __attribute__((weak)) TIM8_BRK_TIM12_IRQHandler();
void __attribute__((weak)) TIM8_UP_TIM13_IRQHandler();
void __attribute__((weak)) TIM8_TRG_COM_TIM14_IRQHandler();
void __attribute__((weak)) TIM8_CC_IRQHandler();
void __attribute__((weak)) DMA1_Stream7_IRQHandler();
void __attribute__((weak)) FMC_IRQHandler();
void __attribute__((weak)) SDMMC1_IRQHandler();
void __attribute__((weak)) TIM5_IRQHandler();
void __attribute__((weak)) SPI3_IRQHandler();
void __attribute__((weak)) UART4_IRQHandler();
void __attribute__((weak)) UART5_IRQHandler();
void __attribute__((weak)) TIM6_DAC_IRQHandler();
void __attribute__((weak)) TIM7_IRQHandler();
void __attribute__((weak)) DMA2_Stream0_IRQHandler();
void __attribute__((weak)) DMA2_Stream1_IRQHandler();
void __attribute__((weak)) DMA2_Stream2_IRQHandler();
void __attribute__((weak)) DMA2_Stream3_IRQHandler();
void __attribute__((weak)) DMA2_Stream4_IRQHandler();
void __attribute__((weak)) ETH_IRQHandler();
void __attribute__((weak)) ETH_WKUP_IRQHandler();
void __attribute__((weak)) CAN2_TX_IRQHandler();
void __attribute__((weak)) CAN2_RX0_IRQHandler();
void __attribute__((weak)) CAN2_RX1_IRQHandler();
void __attribute__((weak)) CAN2_SCE_IRQHandler();
void __attribute__((weak)) OTG_FS_IRQHandler();
void __attribute__((weak)) DMA2_Stream5_IRQHandler();
void __attribute__((weak)) DMA2_Stream6_IRQHandler();
void __attribute__((weak)) DMA2_Stream7_IRQHandler();
void __attribute__((weak)) USART6_IRQHandler();
void __attribute__((weak)) I2C3_EV_IRQHandler();
void __attribute__((weak)) I2C3_ER_IRQHandler();
void __attribute__((weak)) OTG_HS_EP1_OUT_IRQHandler();
void __attribute__((weak)) OTG_HS_EP1_IN_IRQHandler();
void __attribute__((weak)) OTG_HS_WKUP_IRQHandler();
void __attribute__((weak)) OTG_HS_IRQHandler();
void __attribute__((weak)) DCMI_IRQHandler();
void __attribute__((weak)) CRYP_IRQHandler();
void __attribute__((weak)) RNG_IRQHandler();
void __attribute__((weak)) FPU_IRQHandler();
void __attribute__((weak)) UART7_IRQHandler();
void __attribute__((weak)) UART8_IRQHandler();
void __attribute__((weak)) SPI4_IRQHandler();
void __attribute__((weak)) SPI5_IRQHandler();
void __attribute__((weak)) SPI6_IRQHandler();
void __attribute__((weak)) SAI1_IRQHandler();
void __attribute__((weak)) LTDC_IRQHandler();
void __attribute__((weak)) LTDC_ER_IRQHandler();
void __attribute__((weak)) DMA2D_IRQHandler();
void __attribute__((weak)) SAI2_IRQHandler();
void __attribute__((weak)) QUADSPI_IRQHandler();
void __attribute__((weak)) LPTIM1_IRQHandler();
void __attribute__((weak)) CEC_IRQHandler();
void __attribute__((weak)) I2C4_EV_IRQHandler();
void __attribute__((weak)) I2C4_ER_IRQHandler();
void __attribute__((weak)) SPDIF_RX_IRQHandler();
void __attribute__((weak)) DSIHOST_IRQHandler();
void __attribute__((weak)) DFSDM1_FLT0_IRQHandler();
void __attribute__((weak)) DFSDM1_FLT1_IRQHandler();
void __attribute__((weak)) DFSDM1_FLT2_IRQHandler();
void __attribute__((weak)) DFSDM1_FLT3_IRQHandler();
void __attribute__((weak)) SDMMC2_IRQHandler();
void __attribute__((weak)) CAN3_TX_IRQHandler();
void __attribute__((weak)) CAN3_RX0_IRQHandler();
void __attribute__((weak)) CAN3_RX1_IRQHandler();
void __attribute__((weak)) CAN3_SCE_IRQHandler();
void __attribute__((weak)) JPEG_IRQHandler();
void __attribute__((weak)) MDIOS_IRQHandler();

// Stack top, defined in the linker script
extern char _main_stack_top asm("_main_stack_top");

// Interrupt vectors, must be placed @ address 0x00000000
// The extern declaration is required otherwise g++ optimizes it out
extern void (*const __Vectors[])();
void (*const __Vectors[])() __attribute__((section(".isr_vector"))) =
{
    reinterpret_cast<void (*)()>(&_main_stack_top), /* Stack pointer*/
    Reset_Handler,                                  /* Reset Handler */
    NMI_Handler,                                    /* NMI Handler */
    HardFault_Handler,                              /* Hard Fault Handler */
    MemManage_Handler,                              /* MPU Fault Handler */
    BusFault_Handler,                               /* Bus Fault Handler */
    UsageFault_Handler,                             /* Usage Fault Handler */
    0,                                              /* Reserved */
    0,                                              /* Reserved */
    0,                                              /* Reserved */
    0,                                              /* Reserved */
    SVC_Handler,                                    /* SVCall Handler */
    DebugMon_Handler,                               /* Debug Monitor Handler */
    0,                                              /* Reserved */
    PendSV_Handler,                                 /* PendSV Handler */
    SysTick_Handler,                                /* SysTick Handler */

    /* External Interrupts */
    WWDG_IRQHandler,
    PVD_IRQHandler,
    TAMP_STAMP_IRQHandler,
    RTC_WKUP_IRQHandler,
    FLASH_IRQHandler,
    RCC_IRQHandler,
    EXTI0_IRQHandler,
    EXTI1_IRQHandler,
    EXTI2_IRQHandler,
    EXTI3_IRQHandler,
    EXTI4_IRQHandler,
    DMA1_Stream0_IRQHandler,
    DMA1_Stream1_IRQHandler,
    DMA1_Stream2_IRQHandler,
    DMA1_Stream3_IRQHandler,
    DMA1_Stream4_IRQHandler,
    DMA1_Stream5_IRQHandler,
    DMA1_Stream6_IRQHandler,
    ADC_IRQHandler,
    CAN1_TX_IRQHandler,
    CAN1_RX0_IRQHandler,
    CAN1_RX1_IRQHandler,
    CAN1_SCE_IRQHandler,
    EXTI9_5_IRQHandler,
    TIM1_BRK_TIM9_IRQHandler,
    TIM1_UP_TIM10_IRQHandler,
    TIM1_TRG_COM_TIM11_IRQHandler,
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
    OTG_FS_WKUP_IRQHandler,
    TIM8_BRK_TIM12_IRQHandler,
    TIM8_UP_TIM13_IRQHandler,
    TIM8_TRG_COM_TIM14_IRQHandler,
    TIM8_CC_IRQHandler,
    DMA1_Stream7_IRQHandler,
    FMC_IRQHandler,
    SDMMC1_IRQHandler,
    TIM5_IRQHandler,
    SPI3_IRQHandler,
    UART4_IRQHandler,
    UART5_IRQHandler,
    TIM6_DAC_IRQHandler,
    TIM7_IRQHandler,
    DMA2_Stream0_IRQHandler,
    DMA2_Stream1_IRQHandler,
    DMA2_Stream2_IRQHandler,
    DMA2_Stream3_IRQHandler,
    DMA2_Stream4_IRQHandler,
    ETH_IRQHandler,
    ETH_WKUP_IRQHandler,
    CAN2_TX_IRQHandler,
    CAN2_RX0_IRQHandler,
    CAN2_RX1_IRQHandler,
    CAN2_SCE_IRQHandler,
    OTG_FS_IRQHandler,
    DMA2_Stream5_IRQHandler,
    DMA2_Stream6_IRQHandler,
    DMA2_Stream7_IRQHandler,
    USART6_IRQHandler,
    I2C3_EV_IRQHandler,
    I2C3_ER_IRQHandler,
    OTG_HS_EP1_OUT_IRQHandler,
    OTG_HS_EP1_IN_IRQHandler,
    OTG_HS_WKUP_IRQHandler,
    OTG_HS_IRQHandler,
    DCMI_IRQHandler,
    CRYP_IRQHandler,
    RNG_IRQHandler,
    FPU_IRQHandler,
    UART7_IRQHandler,
    UART8_IRQHandler,
    SPI4_IRQHandler,
    SPI5_IRQHandler,
    SPI6_IRQHandler,
    SAI1_IRQHandler,
    LTDC_IRQHandler,
    LTDC_ER_IRQHandler,
    DMA2D_IRQHandler,
    SAI2_IRQHandler,
    QUADSPI_IRQHandler,
    LPTIM1_IRQHandler,
    CEC_IRQHandler,
    I2C4_EV_IRQHandler,
    I2C4_ER_IRQHandler,
    SPDIF_RX_IRQHandler,
    DSIHOST_IRQHandler,
    DFSDM1_FLT0_IRQHandler,
    DFSDM1_FLT1_IRQHandler,
    DFSDM1_FLT2_IRQHandler,
    DFSDM1_FLT3_IRQHandler,
    SDMMC2_IRQHandler,
    CAN3_TX_IRQHandler,
    CAN3_RX0_IRQHandler,
    CAN3_RX1_IRQHandler,
    CAN3_SCE_IRQHandler,
    JPEG_IRQHandler,
    MDIOS_IRQHandler,
};

#pragma weak SysTick_IRQHandler = Default_Handler
#pragma weak WWDG_IRQHandler = Default_Handler
#pragma weak PVD_IRQHandler = Default_Handler
#pragma weak TAMP_STAMP_IRQHandler = Default_Handler
#pragma weak RTC_WKUP_IRQHandler = Default_Handler
#pragma weak FLASH_IRQHandler = Default_Handler
#pragma weak RCC_IRQHandler = Default_Handler
#pragma weak EXTI0_IRQHandler = Default_Handler
#pragma weak EXTI1_IRQHandler = Default_Handler
#pragma weak EXTI2_IRQHandler = Default_Handler
#pragma weak EXTI3_IRQHandler = Default_Handler
#pragma weak EXTI4_IRQHandler = Default_Handler
#pragma weak DMA1_Stream0_IRQHandler = Default_Handler
#pragma weak DMA1_Stream1_IRQHandler = Default_Handler
#pragma weak DMA1_Stream2_IRQHandler = Default_Handler
#pragma weak DMA1_Stream3_IRQHandler = Default_Handler
#pragma weak DMA1_Stream4_IRQHandler = Default_Handler
#pragma weak DMA1_Stream5_IRQHandler = Default_Handler
#pragma weak DMA1_Stream6_IRQHandler = Default_Handler
#pragma weak ADC_IRQHandler = Default_Handler
#pragma weak CAN1_TX_IRQHandler = Default_Handler
#pragma weak CAN1_RX0_IRQHandler = Default_Handler
#pragma weak CAN1_RX1_IRQHandler = Default_Handler
#pragma weak CAN1_SCE_IRQHandler = Default_Handler
#pragma weak EXTI9_5_IRQHandler = Default_Handler
#pragma weak TIM1_BRK_TIM9_IRQHandler = Default_Handler
#pragma weak TIM1_UP_TIM10_IRQHandler = Default_Handler
#pragma weak TIM1_TRG_COM_TIM11_IRQHandler = Default_Handler
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
#pragma weak OTG_FS_WKUP_IRQHandler = Default_Handler
#pragma weak TIM8_BRK_TIM12_IRQHandler = Default_Handler
#pragma weak TIM8_UP_TIM13_IRQHandler = Default_Handler
#pragma weak TIM8_TRG_COM_TIM14_IRQHandler = Default_Handler
#pragma weak TIM8_CC_IRQHandler = Default_Handler
#pragma weak DMA1_Stream7_IRQHandler = Default_Handler
#pragma weak FMC_IRQHandler = Default_Handler
#pragma weak SDMMC1_IRQHandler = Default_Handler
#pragma weak TIM5_IRQHandler = Default_Handler
#pragma weak SPI3_IRQHandler = Default_Handler
#pragma weak UART4_IRQHandler = Default_Handler
#pragma weak UART5_IRQHandler = Default_Handler
#pragma weak TIM6_DAC_IRQHandler = Default_Handler
#pragma weak TIM7_IRQHandler = Default_Handler
#pragma weak DMA2_Stream0_IRQHandler = Default_Handler
#pragma weak DMA2_Stream1_IRQHandler = Default_Handler
#pragma weak DMA2_Stream2_IRQHandler = Default_Handler
#pragma weak DMA2_Stream3_IRQHandler = Default_Handler
#pragma weak DMA2_Stream4_IRQHandler = Default_Handler
#pragma weak ETH_IRQHandler = Default_Handler
#pragma weak ETH_WKUP_IRQHandler = Default_Handler
#pragma weak CAN2_TX_IRQHandler = Default_Handler
#pragma weak CAN2_RX0_IRQHandler = Default_Handler
#pragma weak CAN2_RX1_IRQHandler = Default_Handler
#pragma weak CAN2_SCE_IRQHandler = Default_Handler
#pragma weak OTG_FS_IRQHandler = Default_Handler
#pragma weak DMA2_Stream5_IRQHandler = Default_Handler
#pragma weak DMA2_Stream6_IRQHandler = Default_Handler
#pragma weak DMA2_Stream7_IRQHandler = Default_Handler
#pragma weak USART6_IRQHandler = Default_Handler
#pragma weak I2C3_EV_IRQHandler = Default_Handler
#pragma weak I2C3_ER_IRQHandler = Default_Handler
#pragma weak OTG_HS_EP1_OUT_IRQHandler = Default_Handler
#pragma weak OTG_HS_EP1_IN_IRQHandler = Default_Handler
#pragma weak OTG_HS_WKUP_IRQHandler = Default_Handler
#pragma weak OTG_HS_IRQHandler = Default_Handler
#pragma weak DCMI_IRQHandler = Default_Handler
#pragma weak CRYP_IRQHandler = Default_Handler
#pragma weak RNG_IRQHandler = Default_Handler
#pragma weak FPU_IRQHandler = Default_Handler
#pragma weak UART7_IRQHandler = Default_Handler
#pragma weak UART8_IRQHandler = Default_Handler
#pragma weak SPI4_IRQHandler = Default_Handler
#pragma weak SPI5_IRQHandler = Default_Handler
#pragma weak SPI6_IRQHandler = Default_Handler
#pragma weak SAI1_IRQHandler = Default_Handler
#pragma weak LTDC_IRQHandler = Default_Handler
#pragma weak LTDC_ER_IRQHandler = Default_Handler
#pragma weak DMA2D_IRQHandler = Default_Handler
#pragma weak SAI2_IRQHandler = Default_Handler
#pragma weak QUADSPI_IRQHandler = Default_Handler
#pragma weak LPTIM1_IRQHandler = Default_Handler
#pragma weak CEC_IRQHandler = Default_Handler
#pragma weak I2C4_EV_IRQHandler = Default_Handler
#pragma weak I2C4_ER_IRQHandler = Default_Handler
#pragma weak SPDIF_RX_IRQHandler = Default_Handler
#pragma weak DSIHOST_IRQHandler = Default_Handler
#pragma weak DFSDM1_FLT0_IRQHandler = Default_Handler
#pragma weak DFSDM1_FLT1_IRQHandler = Default_Handler
#pragma weak DFSDM1_FLT2_IRQHandler = Default_Handler
#pragma weak DFSDM1_FLT3_IRQHandler = Default_Handler
#pragma weak SDMMC2_IRQHandler = Default_Handler
#pragma weak CAN3_TX_IRQHandler = Default_Handler
#pragma weak CAN3_RX0_IRQHandler = Default_Handler
#pragma weak CAN3_RX1_IRQHandler = Default_Handler
#pragma weak CAN3_SCE_IRQHandler = Default_Handler
#pragma weak JPEG_IRQHandler = Default_Handler
#pragma weak MDIOS_IRQHandler = Default_Handler
