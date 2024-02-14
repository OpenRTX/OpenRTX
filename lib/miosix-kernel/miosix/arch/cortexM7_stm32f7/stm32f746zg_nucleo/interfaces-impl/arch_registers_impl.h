
#ifndef ARCH_REGISTERS_IMPL_H
#define	ARCH_REGISTERS_IMPL_H

//stm32f7xx.h defines a few macros like __ICACHE_PRESENT, __DCACHE_PRESENT and
//includes core_cm7.h. Do not include core_cm7.h before.
#define STM32F746xx
#include "CMSIS/Device/ST/STM32F7xx/Include/stm32f7xx.h"

#if (__ICACHE_PRESENT != 1) || (__DCACHE_PRESENT != 1)
#error "Wrong include order"
#endif

#include "CMSIS/Device/ST/STM32F7xx/Include/system_stm32f7xx.h"

#define RCC_SYNC() __DSB() //TODO: can this dsb be removed?

#endif	//ARCH_REGISTERS_IMPL_H
