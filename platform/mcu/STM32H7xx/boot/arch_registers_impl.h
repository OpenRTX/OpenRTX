
#ifndef ARCH_REGISTERS_IMPL_H
#define	ARCH_REGISTERS_IMPL_H

//Always include stm32h7xx.h before core_cm4.h, there's some nasty dependency
#include "stm32h7xx.h"
#include "core_cm7.h"
#include "system_stm32h7xx.h"

#define RCC_SYNC() //Workaround for a bug in stm32f42x

#endif	//ARCH_REGISTERS_IMPL_H
