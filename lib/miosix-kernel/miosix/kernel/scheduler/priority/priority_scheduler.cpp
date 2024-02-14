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

#include "priority_scheduler.h"
#include "kernel/error.h"
#include "kernel/process.h"
#include "interfaces/os_timer.h"
#include <limits>

#ifdef SCHED_TYPE_PRIORITY
namespace miosix {

//These are defined in kernel.cpp
extern volatile Thread *runningThread;
extern volatile int kernelRunning;
extern IntrusiveList<SleepData> sleepingList;

//Internal data
static long long nextPeriodicPreemption=std::numeric_limits<long long>::max();

//
// class PriorityScheduler
//

bool PriorityScheduler::PKaddThread(Thread *thread,
        PrioritySchedulerPriority priority)
{
    thread->schedData.priority=priority;
    if(threadList[priority.get()]==nullptr)
    {
        threadList[priority.get()]=thread;
        thread->schedData.next=thread;//Circular list
    } else {
        thread->schedData.next=threadList[priority.get()]->schedData.next;
        threadList[priority.get()]->schedData.next=thread;
    }
    return true;
}

bool PriorityScheduler::PKexists(Thread *thread)
{
    for(int i=PRIORITY_MAX-1;i>=0;i--)
    {
        if(threadList[i]==nullptr) continue;
        Thread *temp=threadList[i];
        for(;;)
        {
            if((temp==thread) && (!temp->flags.isDeleted())) return true;
            temp=temp->schedData.next;
            if(temp==threadList[i]) break;
        }
    }
    return false;
}

void PriorityScheduler::PKremoveDeadThreads()
{
    for(int i=PRIORITY_MAX-1;i>=0;i--)
    {
        if(threadList[i]==nullptr) continue;
        bool first=false;//If false the tail of the list hasn't been calculated
        Thread *tail=nullptr;//Tail of the list
        //Special case: removing first element in the list
        while(threadList[i]->flags.isDeleted())
        {
            if(threadList[i]->schedData.next==threadList[i])
            {
                //Only one element in the list
                //Call destructor manually because of placement new
                void *base=threadList[i]->watermark;
                threadList[i]->~Thread();
                free(base); //Delete ALL thread memory
                threadList[i]=nullptr;
                break;
            }
            //If it is the first time the tail of the list hasn't
            //been calculated
            if(first==false)
            {
                first=true;
                tail=threadList[i];
                while(tail->schedData.next!=threadList[i])
                    tail=tail->schedData.next;
            }
            Thread *d=threadList[i];//Save a pointer to the thread
            threadList[i]=threadList[i]->schedData.next;//Remove from list
            //Fix the tail of the circular list
            tail->schedData.next=threadList[i];
            //Call destructor manually because of placement new
            void *base=d->watermark;
            d->~Thread();
            free(base);//Delete ALL thread memory
        }
        if(threadList[i]==nullptr) continue;
        //If it comes here, the first item is not nullptr, and doesn't have
        //to be deleted General case: removing items not at the first
        //place
        Thread *temp=threadList[i];
        for(;;)
        {
            if(temp->schedData.next==threadList[i]) break;
            if(temp->schedData.next->flags.isDeleted())
            {
                Thread *d=temp->schedData.next;//Save a pointer to the thread
                //Remove from list
                temp->schedData.next=temp->schedData.next->schedData.next;
                //Call destructor manually because of placement new
                void *base=d->watermark;
                d->~Thread();
                free(base);//Delete ALL thread memory
            } else temp=temp->schedData.next;
        }
    }
}

void PriorityScheduler::PKsetPriority(Thread *thread,
        PrioritySchedulerPriority newPriority)
{
    PrioritySchedulerPriority oldPriority=thread->PKgetPriority();
    //First set priority to the new value
    thread->schedData.priority=newPriority;
    //Then remove the thread from its old list
    if(threadList[oldPriority.get()]==thread)
    {
        if(threadList[oldPriority.get()]->schedData.next==
                threadList[oldPriority.get()])
        {
            //Only one element in the list
            threadList[oldPriority.get()]=nullptr;
        } else {
            Thread *tail=threadList[oldPriority.get()];//Tail of the list
            while(tail->schedData.next!=threadList[oldPriority.get()])
                tail=tail->schedData.next;
            //Remove
            threadList[oldPriority.get()]=
                    threadList[oldPriority.get()]->schedData.next;
            //Fix tail of the circular list
            tail->schedData.next=threadList[oldPriority.get()];
        }
    } else {
        //If it comes here, the first item doesn't have to be removed
        //General case: removing item not at the first place
        Thread *temp=threadList[oldPriority.get()];
        for(;;)
        {
            if(temp->schedData.next==threadList[oldPriority.get()])
            {
                //After walking all elements in the list the thread wasn't found
                //This should never happen
                errorHandler(UNEXPECTED);
            }
            if(temp->schedData.next==thread)
            {
                //Remove from list
                temp->schedData.next=temp->schedData.next->schedData.next;
                break;
            } else temp=temp->schedData.next;
        }
    }
    //Last insert the thread in the new list
    if(threadList[newPriority.get()]==nullptr)
    {
        threadList[newPriority.get()]=thread;
        thread->schedData.next=thread;//Circular list
    } else {
        thread->schedData.next=threadList[newPriority.get()]->schedData.next;
        threadList[newPriority.get()]->schedData.next=thread;
    }
}

void PriorityScheduler::IRQsetIdleThread(Thread *idleThread)
{
    idleThread->schedData.priority=-1;
    idle=idleThread;
}

long long PriorityScheduler::IRQgetNextPreemption()
{
    return nextPeriodicPreemption;
}

static long long IRQsetNextPreemption(bool runningIdleThread)
{
    long long first;
    if(sleepingList.empty()) first=std::numeric_limits<long long>::max();
    else first=sleepingList.front()->wakeupTime;

    long long t=IRQgetTime();
    if(runningIdleThread) nextPeriodicPreemption=first;
    else nextPeriodicPreemption=std::min(first,t+MAX_TIME_SLICE);

    //We could not set an interrupt if the sleeping list is empty and runningThread
    //is idle but there's no such hurry to run idle anyway, so why bother?
    internal::IRQosTimerSetInterrupt(nextPeriodicPreemption);
    return t;
}

void PriorityScheduler::IRQfindNextThread()
{
    if(kernelRunning!=0) return;//If kernel is paused, do nothing
    #ifdef WITH_CPU_TIME_COUNTER
    Thread *prev=const_cast<Thread*>(runningThread);
    #endif // WITH_CPU_TIME_COUNTER
    for(int i=PRIORITY_MAX-1;i>=0;i--)
    {
        if(threadList[i]==nullptr) continue;
        Thread *temp=threadList[i]->schedData.next;
        for(;;)
        {
            if(temp->flags.isReady())
            {
                //Found a READY thread, so run this one
                runningThread=temp;
                #ifdef WITH_PROCESSES
                if(const_cast<Thread*>(runningThread)->flags.isInUserspace()==false)
                {
                    ctxsave=runningThread->ctxsave;
                    MPUConfiguration::IRQdisable();
                } else {
                    ctxsave=runningThread->userCtxsave;
                    //A kernel thread is never in userspace, so the cast is safe
                    static_cast<Process*>(runningThread->proc)->mpu.IRQenable();
                }
                #else //WITH_PROCESSES
                ctxsave=temp->ctxsave;
                #endif //WITH_PROCESSES
                //Rotate to next thread so that next time the list is walked
                //a different thread, if available, will be chosen first
                threadList[i]=temp;
                #ifndef WITH_CPU_TIME_COUNTER
                IRQsetNextPreemption(false);
                #else //WITH_CPU_TIME_COUNTER
                auto t=IRQsetNextPreemption(false);
                IRQprofileContextSwitch(prev->timeCounterData,temp->timeCounterData,t);
                #endif //WITH_CPU_TIME_COUNTER
                return;
            } else temp=temp->schedData.next;
            if(temp==threadList[i]->schedData.next) break;
        }
    }
    //No thread found, run the idle thread
    runningThread=idle;
    ctxsave=idle->ctxsave;
    #ifdef WITH_PROCESSES
    MPUConfiguration::IRQdisable();
    #endif //WITH_PROCESSES
    #ifndef WITH_CPU_TIME_COUNTER
    IRQsetNextPreemption(true);
    #else //WITH_CPU_TIME_COUNTER
    auto t=IRQsetNextPreemption(true);
    IRQprofileContextSwitch(prev->timeCounterData,idle->timeCounterData,t);
    #endif //WITH_CPU_TIME_COUNTER
}

Thread *PriorityScheduler::threadList[PRIORITY_MAX]={nullptr};
Thread *PriorityScheduler::idle=nullptr;

} //namespace miosix

#endif //SCHED_TYPE_PRIORITY
