
#ifndef ARCH_REGISTERS_IMPL_H
#define	ARCH_REGISTERS_IMPL_H

//Always include stm32f0xx.h before core_cm0.h, there's some nasty dependency
#include "CMSIS/Device/ST/STM32F0xx/Include/stm32f0xx.h"
#include "CMSIS/Include/core_cm0.h"
#include "CMSIS/Device/ST/STM32F0xx/Include/system_stm32f0xx.h"

#define RCC_SYNC() //Workaround for a bug in stm32f42x

#endif	//ARCH_REGISTERS_IMPL_H
