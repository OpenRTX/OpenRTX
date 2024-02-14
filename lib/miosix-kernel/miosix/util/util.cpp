/***************************************************************************
 *   Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013 by Terraneo Federico *
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

/*
 * Part of Miosix Embedded OS
 * some utilities
 */
#include <cstdio>
#include <malloc.h>
#include "util.h"
#include "kernel/kernel.h"
#include "stdlib_integration/libc_integration.h"
#include "config/miosix_settings.h" //For WATERMARK_FILL and STACK_FILL

using namespace std;

namespace miosix {

//
// MemoryStatists class
//

void MemoryProfiling::print()
{
    unsigned int curFreeStack=getCurrentFreeStack();
    unsigned int absFreeStack=getAbsoluteFreeStack();
    unsigned int stackSize=getStackSize();
    unsigned int curFreeHeap=getCurrentFreeHeap();
    unsigned int absFreeHeap=getAbsoluteFreeHeap();
    unsigned int heapSize=getHeapSize();

    iprintf("Stack memory statistics.\n"
            "Size: %u\n"
            "Used (current/max): %u/%u\n"
            "Free (current/min): %u/%u\n"
            "Heap memory statistics.\n"
            "Size: %u\n"
            "Used (current/max): %u/%u\n"
            "Free (current/min): %u/%u\n",
            stackSize,stackSize-curFreeStack,stackSize-absFreeStack,
            curFreeStack,absFreeStack,
            heapSize,heapSize-curFreeHeap,heapSize-absFreeHeap,
            curFreeHeap,absFreeHeap);
}

unsigned int MemoryProfiling::getStackSize()
{
    return miosix::Thread::getStackSize();
}

unsigned int MemoryProfiling::getAbsoluteFreeStack()
{
    const unsigned int *walk=miosix::Thread::getStackBottom();
    const unsigned int stackSize=miosix::Thread::getStackSize();
    unsigned int count=0;
    while(count<stackSize && *walk==miosix::STACK_FILL)
    {
        //Count unused stack
        walk++;
        count+=4;
    }
    //This takes in account CTXSAVE_ON_STACK. It might underestimate
    //the absolute free stack (by a maximum of CTXSAVE_ON_STACK) but
    //it will never overestimate it, which is important since this
    //member function can be used to select stack sizes.
    if(count<=CTXSAVE_ON_STACK) return 0;
    return count-CTXSAVE_ON_STACK;
}

unsigned int MemoryProfiling::getCurrentFreeStack()
{
    register int *stack_ptr asm("sp");
    const unsigned int *walk=miosix::Thread::getStackBottom();
    unsigned int freeStack=(reinterpret_cast<unsigned int>(stack_ptr)
                          - reinterpret_cast<unsigned int>(walk));
    //This takes into account CTXSAVE_ON_STACK.
    if(freeStack<=CTXSAVE_ON_STACK) return 0;
    return freeStack-CTXSAVE_ON_STACK;
}

unsigned int MemoryProfiling::getHeapSize()
{
    //These extern variables are defined in the linker script
    //Pointer to begin of heap
    extern const char _end asm("_end");
    //Pointer to end of heap
    extern const char _heap_end asm("_heap_end");

    return reinterpret_cast<unsigned int>(&_heap_end)
         - reinterpret_cast<unsigned int>(&_end);
}

unsigned int MemoryProfiling::getAbsoluteFreeHeap()
{
    //This extern variable is defined in the linker script
    //Pointer to end of heap
    extern const char _heap_end asm("_heap_end");

    unsigned int maxHeap=getMaxHeap();

    return reinterpret_cast<unsigned int>(&_heap_end) - maxHeap;
}

unsigned int MemoryProfiling::getCurrentFreeHeap()
{
    struct mallinfo mallocData=_mallinfo_r(__getreent());
    return getHeapSize()-mallocData.uordblks;
}

/**
 * \internal
 * used by memDump
 */
static void memPrint(const char *data, char len)
{
    iprintf("0x%08x | ",reinterpret_cast<unsigned int>(data));
    for(int i=0;i<len;i++) iprintf("%02x ",data[i]);
    for(int i=0;i<(16-len);i++) iprintf("   ");
    iprintf("| ");
    for(int i=0;i<len;i++)
    {
        if((data[i]>=32)&&(data[i]<127)) iprintf("%c",data[i]);
        else iprintf(".");
    }
    iprintf("\n");
}

void memDump(const void *start, int len)
{
    const char *data=reinterpret_cast<const char*>(start);
    while(len>16)
    {
        memPrint(data,16);
        len-=16;
        data+=16;
    }
    if(len>0) memPrint(data,len);
}

#ifdef WITH_CPU_TIME_COUNTER

static void printSingleThreadInfo(Thread *self, Thread *thread,
    int approxDt, long long newTime, long long oldTime, bool isIdleThread,
    bool isNewThread)
{
    long long threadDt = newTime - oldTime;
    int perc = static_cast<int>(threadDt >> 16) * 100 / approxDt;
    iprintf("%p %10lld ns (%2d.%1d%%)", static_cast<void*>(thread), threadDt, perc / 10, perc % 10);
    if(isIdleThread)
    {
        iprintf(" (idle)");
        isIdleThread = false;
    } else if(thread == self) {
        iprintf(" (cur)");
    }
    if(isNewThread) iprintf(" new");
    iprintf("\n");
}

//
// CPUProfiler class
//

long long CPUProfiler::update()
{
    lastSnapshotIndex ^= 1;
    snapshots[lastSnapshotIndex].collect();
    return snapshots[lastSnapshotIndex].time;
}

void CPUProfiler::print()
{
    Snapshot& oldSnap = snapshots[lastSnapshotIndex ^ 1];
    Snapshot& newSnap = snapshots[lastSnapshotIndex];
    std::vector<CPUTimeCounter::Data>& oldInfo = oldSnap.threadData;
    std::vector<CPUTimeCounter::Data>& newInfo = newSnap.threadData;
    long long dt = newSnap.time - oldSnap.time;
    int approxDt = static_cast<int>(dt >> 16) / 10;
    Thread *self = Thread::getCurrentThread();

    iprintf("%d threads, last interval %lld ns\n", newInfo.size(), dt);

    // Compute the difference between oldInfo and newInfo
    auto oldIt = oldInfo.begin();
    auto newIt = newInfo.begin();
    // CPUTimeCounter always returns the idle thread as the first thread
    bool isIdleThread = true;
    while(newIt != newInfo.end() && oldIt != oldInfo.end())
    {
        // Skip old threads that were killed
        while(newIt->thread != oldIt->thread)
        {
            iprintf("%p killed\n", static_cast<void*>(oldIt->thread));
            oldIt++;
        }
        // Found a thread that exists in both lists
        printSingleThreadInfo(self, newIt->thread, approxDt, newIt->usedCpuTime,
            oldIt->usedCpuTime, isIdleThread, false);
        isIdleThread = false;
        newIt++;
        oldIt++;
    }
    // Skip last killed threads
    while(oldIt != oldInfo.end())
    {
        iprintf("%p killed\n", static_cast<void*>(oldIt->thread));
        isIdleThread = false;
        oldIt++;
    }
    // Print info about newly created threads
    while(newIt != newInfo.end())
    {
        printSingleThreadInfo(self, newIt->thread, approxDt, newIt->usedCpuTime,
            0, isIdleThread, true);
        isIdleThread = false;
        newIt++;
    }
}

void CPUProfiler::Snapshot::collect()
{
    // We cannot expand the threadData vector while the kernel is paused.
    //   Therefore we need to expand the vector earlier, pause the kernel, and
    // then check if the number of threads stayed the same. If it didn't,
    // we must unpause the kernel and try again. Otherwise we can fill the
    // vector.
    bool success = false;
    do {
        // Resize the vector with the current number of threads
        unsigned int nThreads = CPUTimeCounter::getThreadCount();
        threadData.resize(nThreads);
        {
            // Pause the kernel!
            PauseKernelLock pLock;

            // If the number of threads changed, try again
            unsigned int nThreads2 = CPUTimeCounter::getThreadCount();
            if(nThreads2 != nThreads)
                continue;
            // Otherwise, stop trying
            success = true;

            // Get the current time now. This makes the time accurate with
            // respect to the data collected, at the cost of making the
            // update interval imprecise (if this timestamp is then used
            // to mantain the update interval)
            time = getTime();
            // Fetch the CPU time data for all threads
            auto i1 = threadData.begin();
            auto i2 = CPUTimeCounter::PKbegin();
            do
                *i1++ = *i2++;
            while(i2 != CPUTimeCounter::PKend());
        }
    } while(!success);
}

void CPUProfiler::thread(long long nsInterval)
{
    CPUProfiler profiler;
    long long t = profiler.update();
    while(!Thread::testTerminate())
    {
        t += nsInterval;
        Thread::nanoSleepUntil(t);
        profiler.update();
        profiler.print();
        iprintf("\n");
    }
}

#endif // WITH_CPU_TIME_COUNTER

} //namespace miosix
