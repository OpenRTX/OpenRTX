
#ifndef ARCH_REGISTERS_IMPL_H
#define	ARCH_REGISTERS_IMPL_H

//Always include stm32l4xx.h before core_cm4.h, there's some nasty dependency
#define STM32L475xx
#include "CMSIS/Device/ST/STM32L4xx/Include/stm32l4xx.h"
#include "CMSIS/Include/core_cm4.h"
#include "CMSIS/Device/ST/STM32L4xx/Include/system_stm32l4xx.h"

#define RCC_SYNC() //Workaround for a bug in stm32f42x

#endif	//ARCH_REGISTERS_IMPL_H
