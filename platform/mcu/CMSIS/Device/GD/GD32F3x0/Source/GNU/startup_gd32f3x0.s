
  .syntax unified
	.cpu cortex-m4
	.fpu softvfp
	.thumb

.global	g_pfnVectors
.global	Default_Handler

/* start address for the initialization values of the .data section.
defined in linker script */
.word	_sidata
/* start address for the .data section. defined in linker script */
.word	_sdata
/* end address for the .data section. defined in linker script */
.word	_edata
/* start address for the .bss section. defined in linker script */
.word	_sbss
/* end address for the .bss section. defined in linker script */
.word	_ebss

.equ  BootRAM,        0xF1E0F85F
/**
 * @brief  This is the code that gets called when the processor first
 *          starts execution following a reset event. Only the absolutely
 *          necessary set is performed, after which the application
 *          supplied main() routine is called.
 * @param  None
 * @retval : None
*/

    .section	.text.Reset_Handler
	.weak	Reset_Handler
	.type	Reset_Handler, %function
Reset_Handler:
  ldr   sp, =_estack    /* Atollic update: set stack pointer */

/* Copy the data segment initializers from flash to SRAM */
  ldr r0, =_sdata
  ldr r1, =_edata
  ldr r2, =_sidata
  movs r3, #0
  b LoopCopyDataInit

CopyDataInit:
  ldr r4, [r2, r3]
  str r4, [r0, r3]
  adds r3, r3, #4

LoopCopyDataInit:
  adds r4, r0, r3
  cmp r4, r1
  bcc CopyDataInit
  
/* Zero fill the bss segment. */
  ldr r2, =_sbss
  ldr r4, =_ebss
  movs r3, #0
  b LoopFillZerobss

FillZerobss:
  str  r3, [r2]
  adds r2, r2, #4

LoopFillZerobss:
  cmp r2, r4
  bcc FillZerobss

/* Call the clock system intitialization function.*/
    bl  SystemInit
/* Call static constructors */
    bl __libc_init_array
/* Call the application's entry point.*/
	bl	main

LoopForever:
    b LoopForever
    
.size	Reset_Handler, .-Reset_Handler

/**
 * @brief  This is the code that gets called when the processor receives an
 *         unexpected interrupt.  This simply enters an infinite loop, preserving
 *         the system state for examination by a debugger.
 *
 * @param  None
 * @retval : None
*/
    .section	.text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
	b	Infinite_Loop
	.size	Default_Handler, .-Default_Handler
/******************************************************************************
*
* The minimal vector table for a Cortex-M4.  Note that the proper constructs
* must be placed on this to ensure that it ends up at physical address
* 0x0000.0000.
*
******************************************************************************/
 	.section	.isr_vector,"a",%progbits
	.type	g_pfnVectors, %object
	.size	g_pfnVectors, .-g_pfnVectors


g_pfnVectors:
	.word	_estack                      // Top of Stack
	.word	Reset_Handler                     // Reset Handler
	.word	NMI_Handler                       // NMI Handler
	.word	HardFault_Handler                 // Hard Fault Handler
	.word	MemManage_Handler                 // MPU Fault Handler
	.word	BusFault_Handler                  // Bus Fault Handler
	.word	UsageFault_Handler                // Usage Fault Handler
	.word	0                                 // Reserved
	.word	0                                 // Reserved
	.word	0                                 // Reserved
	.word	0                                 // Reserved
	.word	SVC_Handler                       // SVCall Handler
	.word	DebugMon_Handler                  // Debug Monitor Handler
	.word	0                                 // Reserved
	.word	PendSV_Handler                    // PendSV Handler
	.word	SysTick_Handler                   // SysTick Handler
	.word   WWDGT_IRQHandler                  // 16:Window Watchdog Timer
    .word   LVD_IRQHandler                    // 17:LVD through EXTI Line detect
    .word   RTC_IRQHandler                    // 18:RTC through EXTI Line
    .word   FMC_IRQHandler                    // 19:FMC
    .word   RCU_CTC_IRQHandler                // 20:RCU and CTC
    .word   EXTI0_1_IRQHandler                // 21:EXTI Line 0 and EXTI Line 1
    .word   EXTI2_3_IRQHandler                // 22:EXTI Line 2 and EXTI Line 3
    .word   EXTI4_15_IRQHandler               // 23:EXTI Line 4 to EXTI Line 15
    .word   TSI_IRQHandler                    // 24:TSI
    .word   DMA_Channel0_IRQHandler           // 25:DMA Channel 0 
    .word   DMA_Channel1_2_IRQHandler         // 26:DMA Channel 1 and DMA Channel 2
    .word   DMA_Channel3_4_IRQHandler         // 27:DMA Channel 3 and DMA Channel 4
    .word   ADC_CMP_IRQHandler                // 28:ADC and Comparator 0-1
    .word   TIMER0_BRK_UP_TRG_COM_IRQHandler  // 29:TIMER0 Break,Update,Trigger and Commutation
    .word   TIMER0_Channel_IRQHandler         // 30:TIMER0 Channel Capture Compare
    .word   TIMER1_IRQHandler                 // 31:TIMER1
    .word   TIMER2_IRQHandler                 // 32:TIMER2
    .word   TIMER5_DAC_IRQHandler             // 33:TIMER5 and DAC
    .word   0                                 // Reserved
    .word   TIMER13_IRQHandler                // 35:TIMER13
    .word   TIMER14_IRQHandler                // 36:TIMER14
    .word   TIMER15_IRQHandler                // 37:TIMER15
    .word   TIMER16_IRQHandler                // 38:TIMER16
    .word   I2C0_EV_IRQHandler                // 39:I2C0 Event
    .word   I2C1_EV_IRQHandler                // 40:I2C1 Event
    .word   SPI0_IRQHandler                   // 41:SPI0
    .word   SPI1_IRQHandler                   // 42:SPI1
    .word   USART0_IRQHandler                 // 43:USART0
    .word   USART1_IRQHandler                 // 44:USART1
    .word   0                                 // Reserved
    .word   CEC_IRQHandler                    // 46:CEC
    .word   0                                 // Reserved
    .word   I2C0_ER_IRQHandler                // 48:I2C0 Error
    .word   0                                 // Reserved
    .word   I2C1_ER_IRQHandler                // 50:I2C1 Error
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   USBFS_WKUP_IRQHandler             // 58:USBFS Wakeup
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   DMA_Channel5_6_IRQHandler         // 64:DMA Channel5 and Channel6 
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   0                                 // Reserved
    .word   USBFS_IRQHandler                  // 83:USBFS

/*******************************************************************************
*
* Provide weak aliases for each Exception handler to the Default_Handler.
* As they are weak aliases, any function with the same name will override
* this definition.
*
*******************************************************************************/

  .weak	NMI_Handler
	.thumb_set NMI_Handler,Default_Handler

  .weak	HardFault_Handler
	.thumb_set HardFault_Handler,Default_Handler

  .weak	MemManage_Handler
	.thumb_set MemManage_Handler,Default_Handler

  .weak	BusFault_Handler
	.thumb_set BusFault_Handler,Default_Handler

	.weak	UsageFault_Handler
	.thumb_set UsageFault_Handler,Default_Handler

	.weak	SVC_Handler
	.thumb_set SVC_Handler,Default_Handler

	.weak	DebugMon_Handler
	.thumb_set DebugMon_Handler,Default_Handler

	.weak	PendSV_Handler
	.thumb_set PendSV_Handler,Default_Handler

	.weak	SysTick_Handler
	.thumb_set SysTick_Handler,Default_Handler

	.weak   WWDGT_IRQHandler
	.thumb_set WWDGT_IRQHandler,Default_Handler
	
    .weak   LVD_IRQHandler
	.thumb_set LVD_IRQHandler,Default_Handler
	
    .weak   RTC_IRQHandler
	.thumb_set RTC_IRQHandler,Default_Handler
	
    .weak   FMC_IRQHandler
	.thumb_set FMC_IRQHandler,Default_Handler
	
    .weak   RCU_CTC_IRQHandler
	.thumb_set RCU_CTC_IRQHandler,Default_Handler
	
    .weak   EXTI0_1_IRQHandler
	.thumb_set EXTI0_1_IRQHandler,Default_Handler
	
    .weak   EXTI2_3_IRQHandler
	.thumb_set EXTI2_3_IRQHandler,Default_Handler
	
    .weak   EXTI4_15_IRQHandler
	.thumb_set EXTI4_15_IRQHandler,Default_Handler
	
    .weak   TSI_IRQHandler
	.thumb_set TSI_IRQHandler,Default_Handler
	
    .weak   DMA_Channel0_IRQHandler
	.thumb_set DMA_Channel0_IRQHandler,Default_Handler
	
    .weak   DMA_Channel1_2_IRQHandler
	.thumb_set DMA_Channel1_2_IRQHandler,Default_Handler
	
    .weak   DMA_Channel3_4_IRQHandler
	.thumb_set DMA_Channel3_4_IRQHandler,Default_Handler
	
    .weak   ADC_CMP_IRQHandler
	.thumb_set ADC_CMP_IRQHandler,Default_Handler
	
    .weak   TIMER0_BRK_UP_TRG_COM_IRQHandler
	.thumb_set TIMER0_BRK_UP_TRG_COM_IRQHandler,Default_Handler
	
    .weak   TIMER0_Channel_IRQHandler
	.thumb_set TIMER0_Channel_IRQHandler,Default_Handler
	
    .weak   TIMER1_IRQHandler
	.thumb_set TIMER1_IRQHandler,Default_Handler
	
    .weak   TIMER2_IRQHandler
	.thumb_set TIMER2_IRQHandler,Default_Handler
	
    .weak   TIMER5_DAC_IRQHandler
	.thumb_set TIMER5_DAC_IRQHandler,Default_Handler
	
    .weak   TIMER13_IRQHandler
	.thumb_set TIMER13_IRQHandler,Default_Handler
	
    .weak   TIMER14_IRQHandler
	.thumb_set TIMER14_IRQHandler,Default_Handler
	
    .weak   TIMER15_IRQHandler
	.thumb_set TIMER15_IRQHandler,Default_Handler
	
    .weak   TIMER16_IRQHandler
	.thumb_set TIMER16_IRQHandler,Default_Handler
	
    .weak   I2C0_EV_IRQHandler
	.thumb_set I2C0_EV_IRQHandler,Default_Handler
	
    .weak   I2C1_EV_IRQHandler
	.thumb_set I2C1_EV_IRQHandler,Default_Handler
	
    .weak   SPI0_IRQHandler
	.thumb_set SPI0_IRQHandler,Default_Handler
	
    .weak   SPI1_IRQHandler
	.thumb_set SPI1_IRQHandler,Default_Handler
	
    .weak   USART0_IRQHandler
	.thumb_set USART0_IRQHandler,Default_Handler
	
    .weak   USART1_IRQHandler
	.thumb_set USART1_IRQHandler,Default_Handler
	
    .weak   CEC_IRQHandler
	.thumb_set CEC_IRQHandler,Default_Handler
	
    .weak   I2C0_ER_IRQHandler
	.thumb_set I2C0_ER_IRQHandler,Default_Handler
	
    .weak   I2C1_ER_IRQHandler
	.thumb_set I2C1_ER_IRQHandler,Default_Handler
	
    .weak   USBFS_WKUP_IRQHandler
	.thumb_set USBFS_WKUP_IRQHandler,Default_Handler
	
    .weak   DMA_Channel5_6_IRQHandler
	.thumb_set DMA_Channel5_6_IRQHandler,Default_Handler
	
    .weak   USBFS_IRQHandler
	.thumb_set USBFS_IRQHandler,Default_Handler
