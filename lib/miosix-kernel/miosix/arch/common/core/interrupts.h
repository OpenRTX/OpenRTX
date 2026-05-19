
//Interrupt code is common for all the cortex M cores, so it has been put here

#ifdef _ARCH_ARM7_LPC2000
#include "interrupts_arm7.h"
#elif defined(_ARCH_CORTEXM0) || defined(_ARCH_CORTEXM3)   \
   || defined(_ARCH_CORTEXM4) || defined(_ARCH_CORTEXM7)
#include "interrupts_cortexMx.h"
#else
#error "Unknown arch"
#endif

// Cortex M0 and M0+ does not have some SCB registers, in order to avoid
// compilation issues a flag is defined to disable code that accesses to 
// registers not present in these families

#if defined(_ARCH_CORTEXM0_STM32)
#define _ARCH_CORTEXM0
#endif
