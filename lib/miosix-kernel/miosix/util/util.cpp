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
#include "config/miosix_settings.h"
#include "arch_settings.h" //For WATERMARK_FILL and STACK_FILL

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

} //namespace miosix
