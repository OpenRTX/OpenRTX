/***************************************************************************
 *   Copyright (C) 2023 by Daniele Cattaneo                                *
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

#include "cpu_time_counter.h"
#include "kernel/kernel.h"

#ifdef WITH_CPU_TIME_COUNTER

using namespace miosix;

Thread *CPUTimeCounter::head = nullptr;
Thread *CPUTimeCounter::tail = nullptr;
volatile unsigned int CPUTimeCounter::nThreads = 0;

long long CPUTimeCounter::getActiveThreadTime()
{
    long long curTime, usedTime, lastAct;
    {
        PauseKernelLock pk;
        curTime = IRQgetTime();
        auto cur = Thread::PKgetCurrentThread();
        usedTime = cur->timeCounterData.usedCpuTime;
        lastAct = cur->timeCounterData.lastActivation;
    }
    return usedTime + (curTime - lastAct);
}

void CPUTimeCounter::PKremoveDeadThreads()
{
    Thread *prev = nullptr;
    Thread *cur = head;
    while(cur)
    {
        if(cur->flags.isDeleted())
        {
            if(prev) prev->timeCounterData.next = cur->timeCounterData.next;
            else head = cur->timeCounterData.next;
            nThreads--;
        } else {
            prev = cur;
        }
        cur = cur->timeCounterData.next;
    }
    tail = prev;
}

#endif // WITH_CPU_TIME_COUNTER
