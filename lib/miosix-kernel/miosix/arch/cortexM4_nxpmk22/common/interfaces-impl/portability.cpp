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

#include "interfaces/portability.h"
#include "kernel/kernel.h"
#include "kernel/error.h"
#include "interfaces/bsp.h"
#include "kernel/scheduler/scheduler.h"
#include "core/interrupts.h"
#include "kernel/process.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cassert>

/**
 * \internal
 * software interrupt routine.
 * Since inside naked functions only assembler code is allowed, this function
 * only calls the ctxsave/ctxrestore macros (which are in assembler), and calls
 * the implementation code in ISR_yield()
 */
void SVC_Handler() __attribute__((naked));
void SVC_Handler()
{
    saveContext();
	//Call ISR_yield(). Name is a C++ mangled name.
    asm volatile("bl _ZN14miosix_private9ISR_yieldEv");
    restoreContext();
}

namespace miosix_private {

/**
 * \internal
 * Called by the software interrupt, yield to next thread
 * Declared noinline to avoid the compiler trying to inline it into the caller,
 * which would violate the requirement on naked functions. Function is not
 * static because otherwise the compiler optimizes it out...
 */
void ISR_yield() __attribute__((noinline));
void ISR_yield()
{
    #ifdef WITH_PROCESSES
    // WARNING: Temporary fix. Rationale:
    // This fix is intended to avoid kernel or process faulting due to
    // another process actions. Consider the case in which a process statically
    // allocates a big array such that there is no space left for saving
    // context data. If the process issues a system call, in the following
    // interrupt the context is saved, but since there is no memory available
    // for all the context data, a mem manage interrupt is set to 'pending'. Then,
    // a fake syscall is issued, based on the value read on the stack (which
    // the process hasn't set due to the memory fault and is likely to be 0);
    // this syscall is usually a yield (due to the value of 0 above),
    // which can cause the scheduling of the kernel thread. At this point,
    // the pending mem fault is issued from the kernel thread, causing the
    // kernel fault and reboot. This is caused by the mem fault interrupt
    // having less priority of the other interrupts.
    // This fix checks if there is a mem fault interrupt pending, and, if so,
    // it clears it and returns before calling the previously mentioned fake
    // syscall.
    if(SCB->SHCSR & (1<<13))
    {
        if(miosix::Thread::IRQreportFault(miosix_private::FaultData(
            MP,0,0)))
        {
            SCB->SHCSR &= ~(1<<13); //Clear MEMFAULTPENDED bit
            return;
        }
    }
    #endif // WITH_PROCESSES
    miosix::Thread::IRQstackOverflowCheck();
    
    #ifdef WITH_PROCESSES
    //If processes are enabled, check the content of r3. If zero then it
    //it is a simple yield, otherwise handle the syscall
    //Note that it is required to use ctxsave and not cur->ctxsave because
    //at this time we do not know if the active context is user or kernel
    unsigned int threadSp=ctxsave[0];
    unsigned int *processStack=reinterpret_cast<unsigned int*>(threadSp);
    if(processStack[3]!=miosix::SYS_YIELD)
        miosix::Thread::IRQhandleSvc(processStack[3]);
    else miosix::Scheduler::IRQfindNextThread();
    #else //WITH_PROCESSES
    miosix::Scheduler::IRQfindNextThread();
    #endif //WITH_PROCESSES
}

void IRQsystemReboot()
{
    NVIC_SystemReset();
}

void initCtxsave(unsigned int *ctxsave, void *(*pc)(void *), unsigned int *sp,
        void *argv)
{
    unsigned int *stackPtr=sp;
    stackPtr--; //Stack is full descending, so decrement first
    *stackPtr=0x01000000; stackPtr--;                                 //--> xPSR
    *stackPtr=reinterpret_cast<unsigned long>(
            &miosix::Thread::threadLauncher); stackPtr--;             //--> pc
    *stackPtr=0xffffffff; stackPtr--;                                 //--> lr
    *stackPtr=0; stackPtr--;                                          //--> r12
    *stackPtr=0; stackPtr--;                                          //--> r3
    *stackPtr=0; stackPtr--;                                          //--> r2
    *stackPtr=reinterpret_cast<unsigned long >(argv); stackPtr--;     //--> r1
    *stackPtr=reinterpret_cast<unsigned long >(pc);                   //--> r0

    ctxsave[0]=reinterpret_cast<unsigned long>(stackPtr);             //--> psp
    //leaving the content of r4-r11 uninitialized
    ctxsave[9]=0xfffffffd; //EXC_RETURN=thread mode, use psp, no floating ops
    //leaving the content of s16-s31 uninitialized
}

void IRQportableStartKernel()
{
    //Enable fault handlers
    SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk
            | SCB_SHCSR_MEMFAULTENA_Msk;
    //Enable traps for division by zero. Trap for unaligned memory access
    //was removed as gcc starting from 4.7.2 generates unaligned accesses by
    //default (https://www.gnu.org/software/gcc/gcc-4.7/changes.html)
    SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;
    NVIC_SetPriorityGrouping(7);//This should disable interrupt nesting
    NVIC_SetPriority(SVCall_IRQn,3);//High priority for SVC (Max=0, min=15)
    NVIC_SetPriority(MemoryManagement_IRQn,2);//Higher priority for MemoryManagement (Max=0, min=15)
    
    #ifdef WITH_PROCESSES
    miosix::IRQenableMPUatBoot();
    #endif //WITH_PROCESSES

    //create a temporary space to save current registers. This data is useless
    //since there's no way to stop the sheduler, but we need to save it anyway.
    unsigned int s_ctxsave[miosix::CTXSAVE_SIZE];
    ctxsave=s_ctxsave;//make global ctxsave point to it
    //Note, we can't use enableInterrupts() now since the call is not mathced
    //by a call to disableInterrupts()
    __enable_fault_irq();
    __enable_irq();
    miosix::Thread::yield();
    //Never reaches here
}

void sleepCpu()
{
    __WFI();
}

} //namespace miosix_private
