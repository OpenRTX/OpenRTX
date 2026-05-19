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
#include "kernel/scheduler/tick_interrupt.h"
#include <algorithm>

/**
 * \internal
 * timer interrupt routine.
 * Since inside naked functions only assembler code is allowed, this function
 * only calls the ctxsave/ctxrestore macros (which are in assembler), and calls
 * the implementation code in ISR_preempt()
 */
void SysTick_Handler()   __attribute__((naked));
void SysTick_Handler()
{
    saveContext();
    //Call ISR_preempt(). Name is a C++ mangled name.
    asm volatile("bl _ZN14miosix_private11ISR_preemptEv");
    restoreContext();
}

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

#ifdef SCHED_TYPE_CONTROL_BASED
/**
 * \internal
 * Auxiliary timer interupt routine.
 * Used for variable lenght bursts in control based scheduler.
 * Since inside naked functions only assembler code is allowed, this function
 * only calls the ctxsave/ctxrestore macros (which are in assembler), and calls
 * the implementation code in ISR_yield()
 */
void TIM2_IRQHandler() __attribute__((naked));
void TIM2_IRQHandler()
{
    saveContext();
    //Call ISR_auxTimer(). Name is a C++ mangled name.
    asm volatile("bl _ZN14miosix_private12ISR_auxTimerEv");
    restoreContext();
}
#endif //SCHED_TYPE_CONTROL_BASED

namespace miosix_private {

/**
 * \internal
 * Called by the timer interrupt, preempt to next thread
 * Declared noinline to avoid the compiler trying to inline it into the caller,
 * which would violate the requirement on naked functions. Function is not
 * static because otherwise the compiler optimizes it out...
 */
void ISR_preempt() __attribute__((noinline));
void ISR_preempt()
{
    IRQstackOverflowCheck();
    miosix::IRQtickInterrupt();
}

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
    IRQstackOverflowCheck();
    miosix::Scheduler::IRQfindNextThread();
}

#ifdef SCHED_TYPE_CONTROL_BASED
/**
 * \internal
 * Auxiliary timer interupt routine.
 * Used for variable lenght bursts in control based scheduler.
 */
void ISR_auxTimer() __attribute__((noinline));
void ISR_auxTimer()
{
    IRQstackOverflowCheck();
    miosix::Scheduler::IRQfindNextThread();//If the kernel is running, preempt
    if(miosix::kernel_running!=0) miosix::tick_skew=true;
    TIM2->SR=0;
}
#endif //SCHED_TYPE_CONTROL_BASED

void IRQstackOverflowCheck()
{
    const unsigned int watermarkSize=miosix::WATERMARK_LEN/sizeof(unsigned int);
    for(unsigned int i=0;i<watermarkSize;i++)
    {
        if(miosix::cur->watermark[i]!=miosix::WATERMARK_FILL)
            miosix::errorHandler(miosix::STACK_OVERFLOW);
    }
    if(miosix::cur->ctxsave[0] < reinterpret_cast<unsigned int>(
            miosix::cur->watermark+watermarkSize))
        miosix::errorHandler(miosix::STACK_OVERFLOW);
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
}

#ifdef WITH_PROCESSES

void initCtxsave(unsigned int *ctxsave, void *(*pc)(void *), unsigned int *sp,
        void *argv, unsigned int *gotBase)
{
    unsigned int *stackPtr=sp;
    stackPtr--; //Stack is full descending, so decrement first
    *stackPtr=0x01000000; stackPtr--;                                 //--> xPSR
    *stackPtr=reinterpret_cast<unsigned long>(pc); stackPtr--;        //--> pc
    *stackPtr=0xffffffff; stackPtr--;                                 //--> lr
    *stackPtr=0; stackPtr--;                                          //--> r12
    *stackPtr=0; stackPtr--;                                          //--> r3
    *stackPtr=0; stackPtr--;                                          //--> r2
    *stackPtr=0; stackPtr--;                                          //--> r1
    *stackPtr=reinterpret_cast<unsigned long >(argv);                 //--> r0

    ctxsave[0]=reinterpret_cast<unsigned long>(stackPtr);             //--> psp
    ctxsave[6]=reinterpret_cast<unsigned long>(gotBase);              //--> r9 
    //leaving the content of r4-r8,r10-r11 uninitialized
}

#endif //WITH_PROCESSES

void IRQportableStartKernel()
{
    //Enable fault handlers
    SCB->SHCSR |= SCB_SHCSR_USGFAULTENA | SCB_SHCSR_BUSFAULTENA
            | SCB_SHCSR_MEMFAULTENA;
    //Enable traps for division by zero. Trap for unaligned memory access
    //was removed as gcc starting from 4.7.2 generates unaligned accesses by
    //default (https://www.gnu.org/software/gcc/gcc-4.7/changes.html)
    SCB->CCR |= SCB_CCR_DIV_0_TRP;
    NVIC_SetPriorityGrouping(7);//This should disable interrupt nesting
    NVIC_SetPriority(SVCall_IRQn,3);//High priority for SVC (Max=0, min=15)
    NVIC_SetPriority(SysTick_IRQn,3);//High priority for SysTick (Max=0, min=15)
    SysTick->LOAD=SystemCoreClock/miosix::TICK_FREQ;
    //Start SysTick, set to generate interrupts
    SysTick->CTRL=SysTick_CTRL_ENABLE | SysTick_CTRL_TICKINT |
            SysTick_CTRL_CLKSOURCE;

    #ifdef SCHED_TYPE_CONTROL_BASED
    AuxiliaryTimer::IRQinit();
    #endif //SCHED_TYPE_CONTROL_BASED
    
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

#ifdef SCHED_TYPE_CONTROL_BASED
void AuxiliaryTimer::IRQinit()
{
    RCC->APB1ENR|=RCC_APB1ENR_TIM2EN;
    RCC_SYNC();
    DBGMCU->CR|=DBGMCU_CR_DBG_TIM2_STOP; //Tim2 stops while debugging
    TIM2->CR1=0; //Upcounter, not started, no special options
    TIM2->CR2=0; //No special options
    TIM2->SMCR=0; //No external trigger
    TIM2->CNT=0; //Clear timer
    TIM2->PSC=(SystemCoreClock/miosix::AUX_TIMER_CLOCK)-1;
    TIM2->ARR=0xffff; //Count from zero to 0xffff
    TIM2->DIER=TIM_DIER_CC1IE; //Enable interrupt on compare
    TIM2->CCR1=0xffff; //This will be initialized later with setValue
    NVIC_SetPriority(TIM2_IRQn,3);//High priority for TIM2 (Max=0, min=15)
    NVIC_EnableIRQ(TIM2_IRQn);
    TIM2->CR1=TIM_CR1_CEN; //Start timer
    //This is very important: without this the prescaler shadow register may
    //not be updated
    TIM2->EGR=TIM_EGR_UG;
}

int AuxiliaryTimer::IRQgetValue()
{
    return static_cast<int>(TIM2->CNT);
}

void AuxiliaryTimer::IRQsetValue(int x)
{
    TIM2->CR1=0; //Stop timer since changing CNT or CCR1 while running fails
    TIM2->CNT=0;
    TIM2->CCR1=static_cast<unsigned short>(std::min(x,0xffff));
    TIM2->CR1=TIM_CR1_CEN; //Start timer again
    //The above instructions cause a spurious if not called within the
    //timer 2 IRQ (This happens if called from an SVC).
    //Clearing the pending bit prevents this spurious interrupt
    TIM2->SR=0;
    NVIC_ClearPendingIRQ(TIM2_IRQn);
}
#endif //SCHED_TYPE_CONTROL_BASED

} //namespace miosix_private
