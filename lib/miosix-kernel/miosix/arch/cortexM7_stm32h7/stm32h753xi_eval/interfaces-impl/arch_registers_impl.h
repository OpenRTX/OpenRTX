
#ifndef ARCH_REGISTERS_IMPL_H
#define	ARCH_REGISTERS_IMPL_H

//stm32h7xx.h defines a few macros like __ICACHE_PRESENT, __DCACHE_PRESENT and
//includes core_cm7.h. Do not include core_cm7.h before.
#define STM32H753xx
#include "CMSIS/Device/ST/STM32H7xx/Include/stm32h7xx.h"

#if (__ICACHE_PRESENT != 1) || (__DCACHE_PRESENT != 1)
#error "Wrong include order"
#endif

#include "CMSIS/Device/ST/STM32H7xx/Include/system_stm32h7xx.h"

#define RCC_SYNC() __DSB()

#endif	//ARCH_REGISTERS_IMPL_H
