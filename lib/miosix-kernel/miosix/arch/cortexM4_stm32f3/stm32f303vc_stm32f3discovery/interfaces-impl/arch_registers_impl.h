
#ifndef ARCH_REGISTERS_IMPL_H
#define	ARCH_REGISTERS_IMPL_H

//Always include stm32f3xx.h before core_cm4.h, there's some nasty dependency
#define STM32F303xC
#include "CMSIS/Device/ST/STM32F3xx/Include/stm32f303xc.h"
#include "CMSIS/Include/core_cm4.h"
#include "CMSIS/Device/ST/STM32F3xx/Include/system_stm32f3xx.h"

#define RCC_SYNC() //Workaround for a bug in stm32f42x

#endif	//ARCH_REGISTERS_IMPL_H
