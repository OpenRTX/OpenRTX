/***************************************************************************
 *   Copyright (C) 2010-2021 by Terraneo Federico                          *
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

#pragma once

#include "interfaces/portability.h"
#include "scheduler.h"

namespace miosix {

//These are a couple of global variables and a function that are part of the
//internal implementation of the kernel and are defined in kernel.cpp
//User code should not know about these nor try to use them.
extern volatile int kernelRunning;///\internal Do not use outside the kernel
extern volatile bool pendingWakeup;///\internal Do not use outside the kernel
extern bool IRQwakeThreads(long long currentTime);///\internal Do not use outside the kernel

/**
 * \param currentTime time in nanoseconds when the timer interrupt fired
 */
inline void IRQtimerInterrupt(long long currentTime)
{
    Thread::IRQstackOverflowCheck();
    bool hptw = IRQwakeThreads(currentTime);
    if(currentTime >= Scheduler::IRQgetNextPreemption() || hptw)
    {
        //End of the burst || a higher priority thread has woken up
        Scheduler::IRQfindNextThread();//If the kernel is running, preempt
        if(kernelRunning!=0) pendingWakeup=true;
    }
}

} //namespace miosix
