/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012 by Terraneo Federico                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/
//Miosix kernel

#ifndef PORTABILITY_IMPL_H
#define PORTABILITY_IMPL_H

#include "interfaces/arch_registers.h"
#include "config/miosix_settings.h"

/**
 * \addtogroup Drivers
 * \{
 */

/*
 * This pointer is used by the kernel, and should not be used by end users.
 * this is a pointer to a location where to store the thread's registers during
 * context switch. It requires C linkage to be used inside asm statement.
 * Registers are saved in the following order:
 * *ctxsave+32 --> r11
 * *ctxsave+28 --> r10
 * *ctxsave+24 --> r9
 * *ctxsave+20 --> r8
 * *ctxsave+16 --> r7
 * *ctxsave+12 --> r6
 * *ctxsave+8  --> r5
 * *ctxsave+4  --> r4
 * *ctxsave+0  --> psp
 */
extern "C" {
extern volatile unsigned int *ctxsave;
}
const int stackPtrOffsetInCtxsave=0; ///< Allows to locate the stack pointer

/**
 * \internal
 * \def saveContext()
 * Save context from an interrupt<br>
 * Must be the first line of an IRQ where a context switch can happen.
 * The IRQ must be "naked" to prevent the compiler from generating context save.
 * 
 * A note on the dmb instruction, without it a race condition was observed
 * between pauseKernel() and IRQfindNextThread(). pauseKernel() uses an strex
 * instruction to store a value in the global variable kernel_running which is
 * tested by the context switch code in IRQfindNextThread(). Without the memory
 * barrier IRQfindNextThread() would occasionally read the previous value and
 * perform a context switch while the kernel was paused, leading to deadlock.
 * The failure was only observed within the exception_test() in the testsuite
 * running on the stm32f429zi_stm32f4discovery.
 */

#define saveContext()                                                        \
{                                                                             \
    asm volatile("push  {lr}             \n\t" /*save lr on MAIN stack*/      \
                 "mrs   r1,  psp         \n\t" /*get PROCESS stack pointer*/  \
                 "ldr   r0,  =ctxsave    \n\t" /*get current context*/        \
                 "ldr   r0,  [r0]        \n\t"                                \
                 "stmia r0!, {r1,r4-r7}  \n\t" /*save PROCESS sp + r4-r7*/    \
                 "mov   r4,  r8          \n\t"                                \
                 "mov   r5,  r9          \n\t"                                \
                 "mov   r6,  r10         \n\t"                                \
                 "mov   r7,  r11         \n\t"                                \
                 "stmia r0!, {r4-r7}     \n\t"                                \
                 "dmb                    \n\t"                                \
                 );                                                           \
}

/**
 * \def restoreContext()
 * Restore context in an IRQ where saveContext() is used. Must be the last line
 * of an IRQ where a context switch can happen. The IRQ must be "naked" to
 * prevent the compiler from generating context restore.
 */

#define restoreContext()                                                     \
{                                                                             \
    asm volatile("ldr   r0,  =ctxsave    \n\t" /*get current context*/        \
                 "ldr   r0,  [r0]        \n\t"                                \
                 "ldmia r0!, {r1,r4-r7}  \n\t" /*pop r8-r11 saving in r4-r7*/ \
                 "msr   psp, r1          \n\t" /*restore PROCESS sp*/         \
                 "ldmia r0,  {r0-r3}     \n\t"                                \
                 "mov   r8,  r0          \n\t"                                \
                 "mov   r9,  r1          \n\t"                                \
                 "mov   r10, r2          \n\t"                                \
                 "mov   r11, r3          \n\t"                                \
                 "pop   {pc}             \n\t" /*return*/                     \
                 );                                                           \
}

/**
 * \}
 */

namespace miosix_private {
    
/**
 * \addtogroup Drivers
 * \{
 */

inline void doYield()
{
    asm volatile("movs r3, #0\n\t"
                 "svc  0"
                 :::"r3");
}

inline void doDisableInterrupts()
{
    // Documentation says __disable_irq() disables all interrupts with
    // configurable priority, so also SysTick and SVC.
    // No need to disable faults with __disable_fault_irq()
    __disable_irq();
    //The new fastDisableInterrupts/fastEnableInterrupts are inline, so there's
    //the need for a memory barrier to avoid aggressive reordering
    asm volatile("":::"memory");
}

inline void doEnableInterrupts()
{
    __enable_irq();
    //The new fastDisableInterrupts/fastEnableInterrupts are inline, so there's
    //the need for a memory barrier to avoid aggressive reordering
    asm volatile("":::"memory");
}

inline bool checkAreInterruptsEnabled()
{
    register int i;
    asm volatile("mrs   %0, primask    \n\t":"=r"(i));
    if(i!=0) return false;
    return true;
}

/**
 * \}
 */

} //namespace miosix_private

#endif //PORTABILITY_IMPL_H
