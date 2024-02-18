
//Interrupt code is common for all the cortex M cores, so it has been put here

#ifdef _ARCH_ARM7_LPC2000
#include "interrupts_arm7.h"
#elif defined(_ARCH_CORTEXM0_STM32F0) || defined(_ARCH_CORTEXM3_STM32F1) \
   || defined(_ARCH_CORTEXM4_STM32F4) || defined(_ARCH_CORTEXM3_STM32F2) \
   || defined(_ARCH_CORTEXM3_STM32L1) || defined(_ARCH_CORTEXM7_STM32F7) \
   || defined(_ARCH_CORTEXM7_STM32H7) || defined(_ARCH_CORTEXM3_EFM32GG) \
   || defined(_ARCH_CORTEXM4_STM32F3) || defined(_ARCH_CORTEXM4_STM32L4) \
   || defined(_ARCH_CORTEXM4_ATSAM4L) || defined(_ARCH_CORTEXM4_NXPMK22)
#include "interrupts_cortexMx.h"
#else
#error "Unknown arch"
#endif
