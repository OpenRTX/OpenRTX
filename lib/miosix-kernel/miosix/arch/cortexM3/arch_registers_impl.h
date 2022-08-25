
#ifndef ARCH_REGISTERS_IMPL_H
#define	ARCH_REGISTERS_IMPL_H

//Always include stm32f10x.h before core_cm3.h, there's some nasty dependency
#include "CMSIS/Device/ST/STM32F10x/Include/stm32f10x.h"
#include "CMSIS/Include/core_cm3.h"
#include "CMSIS/Device/ST/STM32F10x/Include/system_stm32f10x.h"

#define RCC_SYNC() //Workaround for a bug in stm32f42x

#endif	//ARCH_REGISTERS_IMPL_H
