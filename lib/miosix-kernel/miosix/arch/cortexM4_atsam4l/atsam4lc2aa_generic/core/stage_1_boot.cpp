
#include "interfaces/arch_registers.h"
#include "core/interrupts.h" //For the unexpected interrupt call
#include "kernel/stage_2_boot.h"
#include <string.h>

/*
 * startup.cpp
 * ATSAM4L C++ startup.
 * Supports interrupt handlers in C++ without extern "C"
 * Developed by Terraneo Federico, based on ATMEL startup code.
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
     * SystemInit() is called *before* initializing .data and zeroing .bss
     * Despite all startup files provided by ATMEL do the opposite, there are
     * three good reasons to do so:
     * First, the CMSIS specifications say that SystemInit() must not access
     * global variables, so it is actually possible to call it before
     * Second, when running Miosix with the xram linker scripts .data and .bss
     * are placed in the external RAM, so we *must* call SystemInit(), which
     * enables xram, before touching .data and .bss
     * Third, this is a performance improvement since the loops that initialize
     * .data and zeros .bss now run with the CPU at full speed instead of 115kHz
     * Note that it is called before switching stacks because the memory
     * at _heap_end can be unavailable until the external RAM is initialized.
     */
    SystemInit();

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
void __attribute__((weak)) HFLASHC_Handler();
void __attribute__((weak)) PDCA_0_Handler();
void __attribute__((weak)) PDCA_1_Handler();
void __attribute__((weak)) PDCA_2_Handler();
void __attribute__((weak)) PDCA_3_Handler();
void __attribute__((weak)) PDCA_4_Handler();
void __attribute__((weak)) PDCA_5_Handler();
void __attribute__((weak)) PDCA_6_Handler();
void __attribute__((weak)) PDCA_7_Handler();
void __attribute__((weak)) PDCA_8_Handler();
void __attribute__((weak)) PDCA_9_Handler();
void __attribute__((weak)) PDCA_10_Handler();
void __attribute__((weak)) PDCA_11_Handler();
void __attribute__((weak)) PDCA_12_Handler();
void __attribute__((weak)) PDCA_13_Handler();
void __attribute__((weak)) PDCA_14_Handler();
void __attribute__((weak)) PDCA_15_Handler();
void __attribute__((weak)) CRCCU_Handler();
void __attribute__((weak)) USBC_Handler();
void __attribute__((weak)) PEVC_TR_Handler();
void __attribute__((weak)) PEVC_OV_Handler();
void __attribute__((weak)) AESA_Handler();
void __attribute__((weak)) PM_Handler();
void __attribute__((weak)) SCIF_Handler();
void __attribute__((weak)) FREQM_Handler();
void __attribute__((weak)) GPIO_0_Handler();
void __attribute__((weak)) GPIO_1_Handler();
void __attribute__((weak)) GPIO_2_Handler();
void __attribute__((weak)) GPIO_3_Handler();
void __attribute__((weak)) GPIO_4_Handler();
void __attribute__((weak)) GPIO_5_Handler();
void __attribute__((weak)) GPIO_6_Handler();
void __attribute__((weak)) GPIO_7_Handler();
void __attribute__((weak)) GPIO_8_Handler();
void __attribute__((weak)) GPIO_9_Handler();
void __attribute__((weak)) GPIO_10_Handler();
void __attribute__((weak)) GPIO_11_Handler();
void __attribute__((weak)) BPM_Handler();
void __attribute__((weak)) BSCIF_Handler();
void __attribute__((weak)) AST_ALARM_Handler();
void __attribute__((weak)) AST_PER_Handler();
void __attribute__((weak)) AST_OVF_Handler();
void __attribute__((weak)) AST_READY_Handler();
void __attribute__((weak)) AST_CLKREADY_Handler();
void __attribute__((weak)) WDT_Handler();
void __attribute__((weak)) EIC_1_Handler();
void __attribute__((weak)) EIC_2_Handler();
void __attribute__((weak)) EIC_3_Handler();
void __attribute__((weak)) EIC_4_Handler();
void __attribute__((weak)) EIC_5_Handler();
void __attribute__((weak)) EIC_6_Handler();
void __attribute__((weak)) EIC_7_Handler();
void __attribute__((weak)) EIC_8_Handler();
void __attribute__((weak)) IISC_Handler();
void __attribute__((weak)) SPI_Handler();
void __attribute__((weak)) TC00_Handler();
void __attribute__((weak)) TC01_Handler();
void __attribute__((weak)) TC02_Handler();
void __attribute__((weak)) TC10_Handler();
void __attribute__((weak)) TC11_Handler();
void __attribute__((weak)) TC12_Handler();
void __attribute__((weak)) TWIM0_Handler();
void __attribute__((weak)) TWIS0_Handler();
void __attribute__((weak)) TWIM1_Handler();
void __attribute__((weak)) TWIS1_Handler();
void __attribute__((weak)) USART0_Handler();
void __attribute__((weak)) USART1_Handler();
void __attribute__((weak)) USART2_Handler();
void __attribute__((weak)) USART3_Handler();
void __attribute__((weak)) ADCIFE_Handler();
void __attribute__((weak)) DACC_Handler();
void __attribute__((weak)) ACIFC_Handler();
void __attribute__((weak)) ABDACB_Handler();
void __attribute__((weak)) TRNG_Handler();
void __attribute__((weak)) PARC_Handler();
void __attribute__((weak)) CATB_Handler();
void __attribute__((weak)) TWIM2_Handler();
void __attribute__((weak)) TWIM3_Handler();
void __attribute__((weak)) LCDCA_Handler();

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
    HFLASHC_Handler,
    PDCA_0_Handler,
    PDCA_1_Handler,
    PDCA_2_Handler,
    PDCA_3_Handler,
    PDCA_4_Handler,
    PDCA_5_Handler,
    PDCA_6_Handler,
    PDCA_7_Handler,
    PDCA_8_Handler,
    PDCA_9_Handler,
    PDCA_10_Handler,
    PDCA_11_Handler,
    PDCA_12_Handler,
    PDCA_13_Handler,
    PDCA_14_Handler,
    PDCA_15_Handler,
    CRCCU_Handler,
    USBC_Handler,
    PEVC_TR_Handler,
    PEVC_OV_Handler,
    AESA_Handler,
    PM_Handler,
    SCIF_Handler,
    FREQM_Handler,
    GPIO_0_Handler,
    GPIO_1_Handler,
    GPIO_2_Handler,
    GPIO_3_Handler,
    GPIO_4_Handler,
    GPIO_5_Handler,
    GPIO_6_Handler,
    GPIO_7_Handler,
    GPIO_8_Handler,
    GPIO_9_Handler,
    GPIO_10_Handler,
    GPIO_11_Handler,
    BPM_Handler,
    BSCIF_Handler,
    AST_ALARM_Handler,
    AST_PER_Handler,
    AST_OVF_Handler,
    AST_READY_Handler,
    AST_CLKREADY_Handler,
    WDT_Handler,
    EIC_1_Handler,
    EIC_2_Handler,
    EIC_3_Handler,
    EIC_4_Handler,
    EIC_5_Handler,
    EIC_6_Handler,
    EIC_7_Handler,
    EIC_8_Handler,
    IISC_Handler,
    SPI_Handler,
    TC00_Handler,
    TC01_Handler,
    TC02_Handler,
    TC10_Handler,
    TC11_Handler,
    TC12_Handler,
    TWIM0_Handler,
    TWIS0_Handler,
    TWIM1_Handler,
    TWIS1_Handler,
    USART0_Handler,
    USART1_Handler,
    USART2_Handler,
    USART3_Handler,
    ADCIFE_Handler,
    DACC_Handler,
    ACIFC_Handler,
    ABDACB_Handler,
    TRNG_Handler,
    PARC_Handler,
    CATB_Handler,
    0,
    TWIM2_Handler,
    TWIM3_Handler,
    LCDCA_Handler
};

#pragma weak SysTick_Handler = Default_Handler
#pragma weak HFLASHC_Handler = Default_Handler
#pragma weak PDCA_0_Handler = Default_Handler
#pragma weak PDCA_1_Handler = Default_Handler
#pragma weak PDCA_2_Handler = Default_Handler
#pragma weak PDCA_3_Handler = Default_Handler
#pragma weak PDCA_4_Handler = Default_Handler
#pragma weak PDCA_5_Handler = Default_Handler
#pragma weak PDCA_6_Handler = Default_Handler
#pragma weak PDCA_7_Handler = Default_Handler
#pragma weak PDCA_8_Handler = Default_Handler
#pragma weak PDCA_9_Handler = Default_Handler
#pragma weak PDCA_10_Handler = Default_Handler
#pragma weak PDCA_11_Handler = Default_Handler
#pragma weak PDCA_12_Handler = Default_Handler
#pragma weak PDCA_13_Handler = Default_Handler
#pragma weak PDCA_14_Handler = Default_Handler
#pragma weak PDCA_15_Handler = Default_Handler
#pragma weak CRCCU_Handler = Default_Handler
#pragma weak USBC_Handler = Default_Handler
#pragma weak PEVC_TR_Handler = Default_Handler
#pragma weak PEVC_OV_Handler = Default_Handler
#pragma weak AESA_Handler = Default_Handler
#pragma weak PM_Handler = Default_Handler
#pragma weak SCIF_Handler = Default_Handler
#pragma weak FREQM_Handler = Default_Handler
#pragma weak GPIO_0_Handler = Default_Handler
#pragma weak GPIO_1_Handler = Default_Handler
#pragma weak GPIO_2_Handler = Default_Handler
#pragma weak GPIO_3_Handler = Default_Handler
#pragma weak GPIO_4_Handler = Default_Handler
#pragma weak GPIO_5_Handler = Default_Handler
#pragma weak GPIO_6_Handler = Default_Handler
#pragma weak GPIO_7_Handler = Default_Handler
#pragma weak GPIO_8_Handler = Default_Handler
#pragma weak GPIO_9_Handler = Default_Handler
#pragma weak GPIO_10_Handler = Default_Handler
#pragma weak GPIO_11_Handler = Default_Handler
#pragma weak BPM_Handler = Default_Handler
#pragma weak BSCIF_Handler = Default_Handler
#pragma weak AST_ALARM_Handler = Default_Handler
#pragma weak AST_PER_Handler = Default_Handler
#pragma weak AST_OVF_Handler = Default_Handler
#pragma weak AST_READY_Handler = Default_Handler
#pragma weak AST_CLKREADY_Handler = Default_Handler
#pragma weak WDT_Handler = Default_Handler
#pragma weak EIC_1_Handler = Default_Handler
#pragma weak EIC_2_Handler = Default_Handler
#pragma weak EIC_3_Handler = Default_Handler
#pragma weak EIC_4_Handler = Default_Handler
#pragma weak EIC_5_Handler = Default_Handler
#pragma weak EIC_6_Handler = Default_Handler
#pragma weak EIC_7_Handler = Default_Handler
#pragma weak EIC_8_Handler = Default_Handler
#pragma weak IISC_Handler = Default_Handler
#pragma weak SPI_Handler = Default_Handler
#pragma weak TC00_Handler = Default_Handler
#pragma weak TC01_Handler = Default_Handler
#pragma weak TC02_Handler = Default_Handler
#pragma weak TC10_Handler = Default_Handler
#pragma weak TC11_Handler = Default_Handler
#pragma weak TC12_Handler = Default_Handler
#pragma weak TWIM0_Handler = Default_Handler
#pragma weak TWIS0_Handler = Default_Handler
#pragma weak TWIM1_Handler = Default_Handler
#pragma weak TWIS1_Handler = Default_Handler
#pragma weak USART0_Handler = Default_Handler
#pragma weak USART1_Handler = Default_Handler
#pragma weak USART2_Handler = Default_Handler
#pragma weak USART3_Handler = Default_Handler
#pragma weak ADCIFE_Handler = Default_Handler
#pragma weak DACC_Handler = Default_Handler
#pragma weak ACIFC_Handler = Default_Handler
#pragma weak ABDACB_Handler = Default_Handler
#pragma weak TRNG_Handler = Default_Handler
#pragma weak PARC_Handler = Default_Handler
#pragma weak CATB_Handler = Default_Handler
#pragma weak TWIM2_Handler = Default_Handler
#pragma weak TWIM3_Handler = Default_Handler
#pragma weak LCDCA_Handler = Default_Handler
