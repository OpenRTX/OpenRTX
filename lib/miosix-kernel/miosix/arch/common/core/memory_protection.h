
//MPU code is common for all the cortex M cores, so it has been put here

#ifndef MEMORY_PROTECTION_H
#define MEMORY_PROTECTION_H

#if defined(_ARCH_CORTEXM3_STM32F2) || defined(_ARCH_CORTEXM4_STM32F4) \
 || defined(_ARCH_CORTEXM7_STM32F7) || defined(_ARCH_CORTEXM7_STM32H7) \
 || defined(_ARCH_CORTEXM3_EFM32GG) || defined(_ARCH_CORTEXM4_STM32L4)
#include "mpu_cortexMx.h"
#endif

#endif //MEMORY_PROTECTION_H
