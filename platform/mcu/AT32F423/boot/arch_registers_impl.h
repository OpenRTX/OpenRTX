
#ifndef ARCH_REGISTERS_IMPL_H
#define	ARCH_REGISTERS_IMPL_H

//Always include stm32f4xx.h before core_cm4.h, there's some nasty dependency
#include "at32f423.h"
#include "core_cm4.h"
#include "system_at32f423.h"

#define RCC_SYNC() //Workaround for a bug in stm32f42x

#endif	//ARCH_REGISTERS_IMPL_H
