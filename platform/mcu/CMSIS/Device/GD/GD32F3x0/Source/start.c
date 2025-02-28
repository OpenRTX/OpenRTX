/*
Startup file for GD32F3x0

Copyright (c) 2020, Bo Gao <7zlaser@gmail.com>

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of
   conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list
   of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be
   used to endorse or promote products derived from this software without specific
   prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THE
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdint.h>

extern uint32_t _sdata, _edata, _rdata, _sbss, _ebss, _estack;

void main(void);

extern void SystemInit(void);

void Reset_Handler(void)
{
	uint32_t *src, *dst;
	src = &_rdata;
	dst = &_sdata;
	while (dst < &_edata)
		*dst++ = *src++;
	dst = &_sbss;
	while (dst < &_ebss)
		*dst++ = 0;
	SystemInit();
	main();
	while (1)
		;
}

void Null_Handler(void) { return; }
void Loop_Handler(void)
{
	while (1)
		;
}

// Dummy interrupt handlers
void __attribute__((weak, alias("Null_Handler"))) NMI_Handler(void);
void __attribute__((weak, alias("Loop_Handler"))) HardFault_Handler(void);
void __attribute__((weak, alias("Loop_Handler"))) MemManage_Handler(void);
void __attribute__((weak, alias("Loop_Handler"))) BusFault_Handler(void);
void __attribute__((weak, alias("Loop_Handler"))) UsageFault_Handler(void);
void __attribute__((weak, alias("Null_Handler"))) SVC_Handler(void);
void __attribute__((weak, alias("Null_Handler"))) DebugMon_Handler(void);
void __attribute__((weak, alias("Null_Handler"))) PendSV_Handler(void);
void __attribute__((weak, alias("Null_Handler"))) SysTick_Handler(void);
void __attribute__((weak, alias("Null_Handler"))) WWDGT_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) LVD_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) RTC_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) FMC_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) RCU_CTC_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) EXTI0_1_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) EXTI2_3_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) EXTI4_15_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) TSI_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) DMA_Channel0_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) DMA_Channel1_2_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) DMA_Channel3_4_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) ADC_CMP_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) TIMER0_BRK_UP_TRG_COM_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) TIMER0_Channel_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) TIMER1_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) TIMER2_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) TIMER5_DAC_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) TIMER13_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) TIMER14_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) TIMER15_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) TIMER16_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) I2C0_EV_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) I2C1_EV_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) SPI0_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) SPI1_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) USART0_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) USART1_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) CEC_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) I2C0_ER_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) I2C1_ER_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) USBFS_WKUP_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) DMA_Channel5_6_IRQHandler(void);
void __attribute__((weak, alias("Null_Handler"))) USBFS_IRQHandler(void);

// Interrupt vector table
const __attribute__((section(".vectors"))) uint32_t _VECTORS[] =
	{
		(uint32_t)&_estack,			  // Initial SP
		(uint32_t)Reset_Handler,	  // Reset
		(uint32_t)NMI_Handler,		  // NMI
		(uint32_t)HardFault_Handler,  // Hard Fault
		(uint32_t)MemManage_Handler,  // MPU Fault
		(uint32_t)BusFault_Handler,	  // Bus Fault
		(uint32_t)UsageFault_Handler, // Usage Fault
		0, 0, 0, 0,
		(uint32_t)SVC_Handler,		// SVC
		(uint32_t)DebugMon_Handler, // Debug Monitor
		0,
		(uint32_t)PendSV_Handler,					// PendSV
		(uint32_t)SysTick_Handler,					// SysTick
		(uint32_t)WWDGT_IRQHandler,					// WDT
		(uint32_t)LVD_IRQHandler,					// LVD
		(uint32_t)RTC_IRQHandler,					// RTC
		(uint32_t)FMC_IRQHandler,					// FMC
		(uint32_t)RCU_CTC_IRQHandler,				// RCU, CTC
		(uint32_t)EXTI0_1_IRQHandler,				// EXTI 0, 1
		(uint32_t)EXTI2_3_IRQHandler,				// EXTI 2, 3
		(uint32_t)EXTI4_15_IRQHandler,				// EXTI 4~15
		(uint32_t)TSI_IRQHandler,					// TSI
		(uint32_t)DMA_Channel0_IRQHandler,			// DMA 0
		(uint32_t)DMA_Channel1_2_IRQHandler,		// DMA 1, 2
		(uint32_t)DMA_Channel3_4_IRQHandler,		// DMA 3, 4
		(uint32_t)ADC_CMP_IRQHandler,				// ADC, CMP
		(uint32_t)TIMER0_BRK_UP_TRG_COM_IRQHandler, // TIMER 0 ADVCTL
		(uint32_t)TIMER0_Channel_IRQHandler,		// TIMER 0 CC
		(uint32_t)TIMER1_IRQHandler,				// TIMER 1
		(uint32_t)TIMER2_IRQHandler,				// TIMER 2
		(uint32_t)TIMER5_DAC_IRQHandler,			// TIMER 5, DAC
		0,
		(uint32_t)TIMER13_IRQHandler, // TIMER 13
		(uint32_t)TIMER14_IRQHandler, // TIMER 14
		(uint32_t)TIMER15_IRQHandler, // TIMER 15
		(uint32_t)TIMER16_IRQHandler, // TIMER 16
		(uint32_t)I2C0_EV_IRQHandler, // I2C 0 EVT
		(uint32_t)I2C1_EV_IRQHandler, // I2C 1 EVT
		(uint32_t)SPI0_IRQHandler,	  // SPI 0
		(uint32_t)SPI1_IRQHandler,	  // SPI 1
		(uint32_t)USART0_IRQHandler,  // USART 0
		(uint32_t)USART1_IRQHandler,  // USART 1
		0,
		(uint32_t)CEC_IRQHandler, // CEC
		0,
		(uint32_t)I2C0_ER_IRQHandler, // I2C0 ERR
		0,
		(uint32_t)I2C1_ER_IRQHandler, // I2C1 ERR
		0, 0, 0, 0, 0, 0, 0,
		(uint32_t)USBFS_WKUP_IRQHandler, // USBFS WAKE
		0, 0, 0, 0, 0,
		(uint32_t)DMA_Channel5_6_IRQHandler, // DMA 5, 6
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0,
		(uint32_t)USBFS_IRQHandler, // USBFS
};
