
#ifndef ARCH_REGISTERS_IMPL_H
#define	ARCH_REGISTERS_IMPL_H

#define STM32F205xx
#include "CMSIS/Device/ST/STM32F2xx/Include/stm32f2xx.h"
#include "CMSIS/Include/core_cm3.h"
#include "CMSIS/Device/ST/STM32F2xx/Include/system_stm32f2xx.h"

#define RCC_SYNC() //Workaround for a bug in stm32f42x

#endif	//ARCH_REGISTERS_IMPL_H
