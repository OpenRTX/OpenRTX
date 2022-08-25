/***************************************************************************
 *   Copyright (C) 2010 by Terraneo Federico                               *
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

#ifndef TICK_INTERRUPT_H
#define	TICK_INTERRUPT_H

#include "config/miosix_settings.h"
#include "scheduler.h"

namespace miosix {

//These are a couple of global variables and a function that are part of the
//internal implementation of the kernel and are defined in kernel.cpp
//User code should not know about these nor try to use them.
extern volatile int kernel_running;///\internal Do not use outside the kernel
extern volatile bool tick_skew;///\internal Do not use outside the kernel
extern volatile Thread *cur;///\internal Do not use outside the kernel
extern bool IRQwakeThreads();///\internal Do not use outside the kernel

inline void IRQtickInterrupt()
{
    bool woken=IRQwakeThreads();//Increment tick and wake threads,if any
    (void)woken; //Avoid unused variable warning.

    #ifdef SCHED_TYPE_PRIORITY
    //With the priority scheduler every tick causes a context switck
    Scheduler::IRQfindNextThread();//If the kernel is running, preempt
    if(kernel_running!=0) tick_skew=true;
    #elif defined(SCHED_TYPE_CONTROL_BASED)
    //Normally, with the control based scheduler, preemptions do not happen
    //here, but in the auxiliary timer interrupt to take into account variable
    //bursts. But there is one exception: when a thread wakes up from sleep
    //and the idle thread is running.
    if(woken && cur==ControlScheduler::IRQgetIdleThread())
    {
        Scheduler::IRQfindNextThread();
        if(kernel_running!=0) tick_skew=true;
    }
    #elif defined(SCHED_TYPE_EDF)
    //With the EDF scheduler a preemption happens only if a thread with a closer
    //deadline appears. So by deafult there is no need to call the scheduler;
    //only if some threads were woken, they may have closer deadlines
    if(woken)
    {
        Scheduler::IRQfindNextThread();
        if(kernel_running!=0) tick_skew=true;
    }
    #endif
}

}

#endif //TICK_INTERRUPT_H
