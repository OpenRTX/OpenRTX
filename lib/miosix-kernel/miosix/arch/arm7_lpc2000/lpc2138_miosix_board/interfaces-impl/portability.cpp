/***************************************************************************
 *   Copyright (C) 2008-2021 by Terraneo Federico                          *
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

#include "LPC213x.h"
#include "interfaces/portability.h"
#include "interfaces/delays.h"
#include "kernel/kernel.h"
#include "kernel/error.h"
#include "miosix.h"
#include "portability_impl.h"
#include "kernel/scheduler/scheduler.h"
#include <algorithm>

using namespace std;

namespace miosix_private {

/**
 * \internal
 * Called by the software interrupt, yield to next thread
 * Declared noinline to avoid the compiler trying to inline it into the caller,
 * which would violate the requirement on naked functions.
 * Function is not static because otherwise the compiler optimizes it out...
 */
void ISR_yield() __attribute__((noinline));
void ISR_yield()
{
    miosix::Thread::IRQstackOverflowCheck();
    miosix::Scheduler::IRQfindNextThread();
}

/**
 * \internal
 * software interrupt routine.
 * Since inside naked functions only assembler code is allowed, this function
 * only calls the ctxsave/ctxrestore macros (which are in assembler), and calls
 * the implementation code in ISR_yield()
 */
extern "C" void kernel_SWI_Routine()   __attribute__ ((interrupt("SWI"),naked));
extern "C" void kernel_SWI_Routine()
{
    saveContextFromSwi();
    //Call ISR_yield(). Name is a C++ mangled name.
    asm volatile("bl _ZN14miosix_private9ISR_yieldEv");
    restoreContext();
}

void IRQsystemReboot()
{    
    //Jump to reset vector
    asm volatile("ldr pc, =0"::);
}

void initCtxsave(unsigned int *ctxsave, void *(*pc)(void *), unsigned int *sp,
            void *argv)
{
    ctxsave[0]=reinterpret_cast<unsigned int>(pc);// First function arg is passed in r0
    ctxsave[1]=reinterpret_cast<unsigned int>(argv);
    ctxsave[2]=0;
    ctxsave[3]=0;
    ctxsave[4]=0;
    ctxsave[5]=0;
    ctxsave[6]=0;
    ctxsave[7]=0;
    ctxsave[8]=0;
    ctxsave[9]=0;
    ctxsave[10]=0;
    ctxsave[11]=0;
    ctxsave[12]=0;
    ctxsave[13]=reinterpret_cast<unsigned int>(sp);//Initialize the thread's stack pointer
    ctxsave[14]=0xffffffff;//threadLauncher never returns, so lr is not important
    //Initialize the thread's program counter to the beginning of the entry point
    ctxsave[15]=reinterpret_cast<unsigned int>(&miosix::Thread::threadLauncher);
    ctxsave[16]=0x1f;//thread starts in system mode with irq and fiq enabled.
}

void IRQportableStartKernel()
{
    //create a temporary space to save current registers. This data is useless
    //since there's no way to stop the sheduler, but we need to save it anyway.
    unsigned int s_ctxsave[miosix::CTXSAVE_SIZE];
    ctxsave=s_ctxsave;//make global ctxsave point to it
    miosix::Thread::yield();//Note that this automatically enables interrupts
    //Never reaches here
}

//IDL bit in PCON register
#define IDL (1<<0)

void sleepCpu()
{
    PCON|=IDL;
}

} //namespace miosix_private
