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

#include "control_scheduler.h"
#include "kernel/error.h"
#include "kernel/process.h"
#include "interfaces/os_timer.h"
#include <limits>

using namespace std;

#ifdef SCHED_TYPE_CONTROL_BASED

namespace miosix {

//These are defined in kernel.cpp
extern volatile Thread *runningThread;
extern volatile int kernelRunning;
extern IntrusiveList<SleepData> sleepingList;

//Internal
static long long burstStart=0;
static long long nextPreemption=numeric_limits<long long>::max();

// Should be called when the running thread is the idle thread
static inline void IRQsetNextPreemptionForIdle()
{
    if(sleepingList.empty()) nextPreemption=numeric_limits<long long>::max();
    else nextPreemption=sleepingList.front()->wakeupTime;
    #ifdef WITH_CPU_TIME_COUNTER
    burstStart=IRQgetTime();
    #endif // WITH_CPU_TIME_COUNTER
    //We could not set an interrupt if the sleeping list is empty but there's
    //no such hurry to run idle anyway, so why bother?
    internal::IRQosTimerSetInterrupt(nextPreemption);
}

// Should be called for threads other than idle thread
static inline void IRQsetNextPreemption(long long burst)
{
    long long firstWakeupInList;
    if(sleepingList.empty()) firstWakeupInList=numeric_limits<long long>::max();
    else firstWakeupInList=sleepingList.front()->wakeupTime;
    burstStart=IRQgetTime();
    nextPreemption=min(firstWakeupInList,burstStart+burst);
    internal::IRQosTimerSetInterrupt(nextPreemption);
}

#ifndef SCHED_CONTROL_MULTIBURST

//
// class ControlScheduler
//

bool ControlScheduler::PKaddThread(Thread *thread,
        ControlSchedulerPriority priority)
{
    #ifdef SCHED_CONTROL_FIXED_POINT
    if(threadListSize>=64) return false;
    #endif //SCHED_CONTROL_FIXED_POINT
    thread->schedData.priority=priority;
    {
        //Note: can't use FastInterruptDisableLock here since this code is
        //also called *before* the kernel is started.
        //Using FastInterruptDisableLock would enable interrupts prematurely
        //and cause all sorts of misterious crashes
        InterruptDisableLock dLock;
        thread->schedData.next=threadList;
        threadList=thread;
        threadListSize++;
        SP_Tr+=bNominal; //One thread more, increase round time
        IRQrecalculateAlfa();
    }
    return true;
}

bool ControlScheduler::PKexists(Thread *thread)
{
    if(thread==nullptr) return false;
    for(Thread *it=threadList;it!=nullptr;it=it->schedData.next)
    {
       if(it==thread)
       {
           if(it->flags.isDeleted()) return false; //Found, but deleted
           return true;
       }
    }
    return false;
}

void ControlScheduler::PKremoveDeadThreads()
{
    //Special case, threads at the head of the list
    while(threadList!=nullptr && threadList->flags.isDeleted())
    {
        Thread *toBeDeleted=threadList;
        {
            FastInterruptDisableLock dLock;
            threadList=threadList->schedData.next;
            threadListSize--;
            SP_Tr-=bNominal; //One thread less, reduce round time
        }
        void *base=toBeDeleted->watermark;
        toBeDeleted->~Thread();
        free(base); //Delete ALL thread memory
    }
    if(threadList!=nullptr)
    {
        //General case, delete threads not at the head of the list
        for(Thread *it=threadList;it->schedData.next!=nullptr;it=it->schedData.next)
        {
            if(it->schedData.next->flags.isDeleted()==false) continue;
            Thread *toBeDeleted=it->schedData.next;
            {
                FastInterruptDisableLock dLock;
                it->schedData.next=it->schedData.next->schedData.next;
                threadListSize--;
                SP_Tr-=bNominal; //One thread less, reduce round time
            }
            void *base=toBeDeleted->watermark;
            toBeDeleted->~Thread();
            free(base); //Delete ALL thread memory
        }
    }
    {
        FastInterruptDisableLock dLock;
        IRQrecalculateAlfa();
    }
}

void ControlScheduler::PKsetPriority(Thread *thread,
        ControlSchedulerPriority newPriority)
{
    thread->schedData.priority=newPriority;
    {
        FastInterruptDisableLock dLock;
        IRQrecalculateAlfa();
    }
}

void ControlScheduler::IRQsetIdleThread(Thread *idleThread)
{
    idleThread->schedData.priority=-1;
    idle=idleThread;
    //Initializing curInRound to end() so that the first time
    //IRQfindNextThread() is called the scheduling algorithm runs
    if(threadListSize!=1) errorHandler(UNEXPECTED);
    curInRound=nullptr;
}

Thread *ControlScheduler::IRQgetIdleThread()
{
    return idle;
}

long long ControlScheduler::IRQgetNextPreemption()
{
    return nextPreemption;
}

void ControlScheduler::IRQfindNextThread()
{
    if(kernelRunning!=0) return;//If kernel is paused, do nothing
    #ifdef WITH_CPU_TIME_COUNTER
    Thread *prev=const_cast<Thread*>(runningThread);
    #endif // WITH_CPU_TIME_COUNTER

    if(runningThread!=idle)
    {
        //Not preempting from the idle thread, store actual burst time of
        //the preempted thread
        int Tp=static_cast<int>(IRQgetTime()-burstStart);
        runningThread->schedData.Tp=Tp;
        Tr+=Tp;
    }

    //Find next thread to run
    for(;;)
    {
        if(curInRound!=nullptr) curInRound=curInRound->schedData.next;
        if(curInRound==nullptr) //Note: do not replace with an else
        {
            //Check these two statements:
            //- If all threads are not ready, the scheduling algorithm must be
            //  paused and the idle thread is run instead
            //- If the inner integral regulator of all ready threads saturated
            //  then the integral regulator of the outer regulator must stop
            //  increasing because the set point cannot be attained anyway.
            bool allThreadNotReady=true;
            bool allReadyThreadsSaturated=true;
            for(Thread *it=threadList;it!=nullptr;it=it->schedData.next)
            {
                if(it->flags.isReady())
                {
                    allThreadNotReady=false;
                    if(it->schedData.bo<bMax*multFactor)
                    {
                        allReadyThreadsSaturated=false;
                        //Found a counterexample for both statements,
                        //no need to scan the list further.
                        break;
                    }
                }
            }
            if(allThreadNotReady)
            {
                //No thread is ready, run the idle thread

                //This is very important: the idle thread can *remove* dead
                //threads from threadList, so it can invalidate iterators
                //to any element except theadList.end()
                curInRound=nullptr;
                runningThread=idle;
                ctxsave=runningThread->ctxsave;
                #ifdef WITH_PROCESSES
                miosix_private::MPUConfiguration::IRQdisable();
                #endif
                IRQsetNextPreemptionForIdle();
                #ifdef WITH_CPU_TIME_COUNTER
                IRQprofileContextSwitch(prev->timeCounterData,
                                        idle->timeCounterData,burstStart);
                #endif //WITH_CPU_TIME_COUNTER
                return;
            }

            //End of round reached, run scheduling algorithm
            curInRound=threadList;
            IRQrunRegulator(allReadyThreadsSaturated);
        }

        if(curInRound->flags.isReady())
        {
            //Found a READY thread, so run this one
            runningThread=curInRound;
            #ifdef WITH_PROCESSES
            if(const_cast<Thread*>(runningThread)->flags.isInUserspace()==false)
            {
                ctxsave=runningThread->ctxsave;
                miosix_private::MPUConfiguration::IRQdisable();
            } else {
                ctxsave=runningThread->userCtxsave;
                //A kernel thread is never in userspace, so the cast is safe
                static_cast<Process*>(runningThread->proc)->mpu.IRQenable();
            }
            #else //WITH_PROCESSES
            ctxsave=runningThread->ctxsave;
            #endif //WITH_PROCESSES
            IRQsetNextPreemption(curInRound->schedData.bo/multFactor);
            #ifdef WITH_CPU_TIME_COUNTER
            IRQprofileContextSwitch(prev->timeCounterData,
                                    curInRound->timeCounterData,burstStart);
            #endif //WITH_CPU_TIME_COUNTER
            return;
        } else {
            //If we get here we have a non ready thread that cannot run,
            //so regardless of the burst calculated by the scheduler
            //we do not run it and set Tp to zero.
            curInRound->schedData.Tp=0;
        }
    }
}

void ControlScheduler::IRQwaitStatusHook(Thread* t)
{
    #ifdef ENABLE_FEEDFORWARD
    IRQrecalculateAlfa();
    #endif //ENABLE_FEEDFORWARD
}

void ControlScheduler::IRQrecalculateAlfa()
{
    //Sum of all priorities of all threads
    //Note that since priority goes from 0 to PRIORITY_MAX-1
    //but priorities we need go from 1 to PRIORITY_MAX we need to add one
    unsigned int sumPriority=0;
    for(Thread *it=threadList;it!=nullptr;it=it->schedData.next)
    {
        #ifdef ENABLE_FEEDFORWARD
        //Count only ready threads
        if(it->flags.isReady())
            sumPriority+=it->schedData.priority.get()+1;//Add one
        #else //ENABLE_FEEDFORWARD
        //Count all threads
        sumPriority+=it->schedData.priority.get()+1;//Add one
        #endif //ENABLE_FEEDFORWARD
    }
    //This can happen when ENABLE_FEEDFORWARD is set and no thread is ready
    if(sumPriority==0) return;
    #ifndef SCHED_CONTROL_FIXED_POINT
    float base=1.0f/((float)sumPriority);
    for(Thread *it=threadList;it!=nullptr;it=it->schedData.next)
    {
        #ifdef ENABLE_FEEDFORWARD
        //Assign zero bursts to blocked threads
        if(it->flags.isReady())
        {
            it->schedData.alfa=base*((float)(it->schedData.priority.get()+1));
        } else {
            it->schedData.alfa=0;
        }
        #else //ENABLE_FEEDFORWARD
        //Assign bursts irrespective of thread blocking status
        it->schedData.alfa=base*((float)(it->schedData.priority.get()+1));
        #endif //ENABLE_FEEDFORWARD
    }
    #else //FIXED_POINT_MATH
    //Sum of all alfa is maximum value for an unsigned short
    unsigned int base=4096/sumPriority;
    for(Thread *it=threadList;it!=nullptr;it=it->schedData.next)
    {
        #ifdef ENABLE_FEEDFORWARD
        //Assign zero bursts to blocked threads
        if(it->flags.isReady())
        {
            it->schedData.alfa=base*(it->schedData.priority.get()+1);
        } else {
            it->schedData.alfa=0;
        }
        #else //ENABLE_FEEDFORWARD
        //Assign bursts irrespective of thread blocking status
        it->schedData.alfa=base*(it->schedData.priority.get()+1);
        #endif //ENABLE_FEEDFORWARD
    }
    #endif //FIXED_POINT_MATH
    reinitRegulator=true;
}

void ControlScheduler::IRQrunRegulator(bool allReadyThreadsSaturated)
{
    using namespace std;
    #ifdef SCHED_CONTROL_FIXED_POINT
    //The fixed point scheduler may overflow if Tr is higher than this
    Tr=min(Tr,524287);
    #endif //FIXED_POINT_MATH
    #ifdef ENABLE_REGULATOR_REINIT
    if(reinitRegulator==false)
    {
    #endif //ENABLE_REGULATOR_REINIT
        int eTr=SP_Tr-Tr;
        #ifndef SCHED_CONTROL_FIXED_POINT
        int bc=bco+static_cast<int>(krr*eTr-krr*zrr*eTro);
        #else //FIXED_POINT_MATH
        //Tr is clamped to 524287, so eTr uses at most 19bits. Considering
        //the 31bits of a signed int, we have 12bits free.
        const int fixedKrr=static_cast<int>(krr*2048);
        const int fixedKrrZrr=static_cast<int>(krr*zrr*1024);
        int bc=bco+(fixedKrr*eTr)/2048-(fixedKrrZrr*eTro)/1024;
        #endif //FIXED_POINT_MATH
        if(allReadyThreadsSaturated)
        {
            //If all inner regulators reached upper saturation,
            //allow only a decrease in the burst correction.
            if(bc<bco) bco=bc;
        } else bco=bc;

        bco=min<int>(max(bco,-Tr),bMax*threadListSize);
        #ifndef SCHED_CONTROL_FIXED_POINT
        float nextRoundTime=static_cast<float>(Tr+bco);
        #else //FIXED_POINT_MATH
        unsigned int nextRoundTime=Tr+bco; //Bounded to 20bits
        #endif //FIXED_POINT_MATH
        eTro=eTr;
        Tr=0;//Reset round time
        for(Thread *it=threadList;it!=nullptr;it=it->schedData.next)
        {
            //Recalculate per thread set point
            #ifndef SCHED_CONTROL_FIXED_POINT
            it->schedData.SP_Tp=static_cast<int>(
                    it->schedData.alfa*nextRoundTime);
            #else //FIXED_POINT_MATH
            //nextRoundTime is bounded to 20bits, alfa to 12bits,
            //so the multiplication fits in 32bits
            it->schedData.SP_Tp=(it->schedData.alfa*nextRoundTime)/4096;
            #endif //FIXED_POINT_MATH

            //Run each thread internal regulator
            int eTp=it->schedData.SP_Tp - it->schedData.Tp;
            //note: since b and bo contain the real value multiplied by
            //multFactor, this equals b=bo+eTp/multFactor.
            int b=it->schedData.bo + eTp;
            //saturation
            it->schedData.bo=min(max(b,bMin*multFactor),bMax*multFactor);
        }
    #ifdef ENABLE_REGULATOR_REINIT
    } else {
        reinitRegulator=false;
        Tr=0;//Reset round time
        //Reset state of the external regulator
        eTro=0;
        bco=0;

        for(Thread *it=threadList;it!=nullptr;it=it->schedData.next)
        {
            //Recalculate per thread set point
            #ifndef SCHED_CONTROL_FIXED_POINT
            it->schedData.SP_Tp=static_cast<int>(it->schedData.alfa*SP_Tr);
            #else //FIXED_POINT_MATH
            //SP_Tr is bounded to 20bits, alfa to 12bits,
            //so the multiplication fits in 32bits
            it->schedData.SP_Tp=(it->schedData.alfa*SP_Tr)/4096;
            #endif //FIXED_POINT_MATH

            int b=it->schedData.SP_Tp*multFactor;
            it->schedData.bo=min(max(b,bMin*multFactor),bMax*multFactor);
        }
    }
    #endif //ENABLE_REGULATOR_REINIT
}

Thread *ControlScheduler::threadList=nullptr;
unsigned int ControlScheduler::threadListSize=0;
Thread *ControlScheduler::curInRound=nullptr;
Thread *ControlScheduler::idle=nullptr;
int ControlScheduler::SP_Tr=0;
int ControlScheduler::Tr=bNominal;
int ControlScheduler::bco=0;
int ControlScheduler::eTro=0;
bool ControlScheduler::reinitRegulator=false;
}

#else //SCHED_CONTROL_MULTIBURST

//Internal
static IntrusiveList<ThreadsListItem> activeThreads;
static IntrusiveList<ThreadsListItem>::iterator curInRound=activeThreads.end();

//
// class ControlScheduler
//

static inline void addThreadToActiveList(ThreadsListItem *atlEntry)
{
    
    switch(atlEntry->t->IRQgetPriority().getRealtime())
    {
        case ControlRealtimePriority::REALTIME_PRIORITY_IMMEDIATE:
            activeThreads.insert(curInRound,atlEntry);
            curInRound--; curInRound--;
            break;
        case ControlRealtimePriority::REALTIME_PRIORITY_NEXT_BURST:
        {
            auto temp=curInRound;
            activeThreads.insert(++temp,atlEntry);
            break;
        }
        default:
            activeThreads.push_back(atlEntry);
    }
}

static inline void remThreadfromActiveList(ThreadsListItem *atlEntry)
{
    if(*curInRound==atlEntry) curInRound++;
    activeThreads.removeFast(atlEntry);
}

bool ControlScheduler::PKaddThread(Thread *thread,
        ControlSchedulerPriority priority)
{
    #ifdef SCHED_CONTROL_FIXED_POINT
    if(threadListSize>=64) return false;
    #endif //SCHED_CONTROL_FIXED_POINT
    thread->schedData.priority=priority;
    thread->schedData.atlEntry.t = thread;
    {
        //Note: can't use FastInterruptDisableLock here since this code is
        //also called *before* the kernel is started.
        //Using FastInterruptDisableLock would enable interrupts prematurely
        //and cause all sorts of misterious crashes
        InterruptDisableLock dLock;
        thread->schedData.next=threadList;
        threadList=thread;
        threadListSize++;
        SP_Tr+=bNominal; //One thread more, increase round time
        // Insert the thread in activeThreads list according to its real-time
        // priority
        if(thread->flags.isReady())
        {
            addThreadToActiveList(&thread->schedData.atlEntry);
            thread->schedData.lastReadyStatus=true;
        } else {
            thread->schedData.lastReadyStatus=false;
        }
        
        IRQrecalculateAlfa();
    }
    return true;
}

bool ControlScheduler::PKexists(Thread *thread)
{
    if(thread==nullptr) return false;
    for(Thread *it=threadList;it!=nullptr;it=it->schedData.next)
    {
       if(it==thread)
       {
           if(it->flags.isDeleted()) return false; //Found, but deleted
           return true;
       }
    }
    return false;
}

void ControlScheduler::PKremoveDeadThreads()
{
    //Special case, threads at the head of the list
    while(threadList!=nullptr && threadList->flags.isDeleted())
    {
        Thread *toBeDeleted=threadList;
        {
            FastInterruptDisableLock dLock;
            threadList=threadList->schedData.next;
            threadListSize--;
            SP_Tr-=bNominal; //One thread less, reduce round time
        }
        void *base=toBeDeleted->watermark;
        toBeDeleted->~Thread();
        free(base); //Delete ALL thread memory
    }
    if(threadList!=nullptr)
    {
        //General case, delete threads not at the head of the list
        for(Thread *it=threadList;it->schedData.next!=nullptr;it=it->schedData.next)
        {
            if(it->schedData.next->flags.isDeleted()==false) continue;
            Thread *toBeDeleted=it->schedData.next;
            {
                FastInterruptDisableLock dLock;
                it->schedData.next=it->schedData.next->schedData.next;
                threadListSize--;
                SP_Tr-=bNominal; //One thread less, reduce round time
            }
            void *base=toBeDeleted->watermark;
            toBeDeleted->~Thread();
            free(base); //Delete ALL thread memory
        }
    }
    {
        FastInterruptDisableLock dLock;
        IRQrecalculateAlfa();
    }
}

void ControlScheduler::PKsetPriority(Thread *thread,
        ControlSchedulerPriority newPriority)
{
    thread->schedData.priority=newPriority;
    {
        FastInterruptDisableLock dLock;
        IRQrecalculateAlfa();
    }
}

void ControlScheduler::IRQsetIdleThread(Thread *idleThread)
{
    idleThread->schedData.priority=-1;
    idle=idleThread;
    //Initializing curInRound to end() so that the first time
    //IRQfindNextThread() is called the scheduling algorithm runs
    if(threadListSize!=1) errorHandler(UNEXPECTED);
    curInRound=activeThreads.end();
}

Thread *ControlScheduler::IRQgetIdleThread()
{
    return idle;
}

long long ControlScheduler::IRQgetNextPreemption()
{
    return nextPreemption;
}

void ControlScheduler::IRQfindNextThread()
{
    if(kernelRunning!=0) return;//If kernel is paused, do nothing
    #ifdef WITH_CPU_TIME_COUNTER
    Thread *prev=const_cast<Thread*>(runningThread);
    #endif // WITH_CPU_TIME_COUNTER

    if(runningThread!=idle)
    {
        //Not preempting from the idle thread, store actual burst time of
        //the preempted thread
        int Tp=static_cast<int>(IRQgetTime()-burstStart);
        runningThread->schedData.Tp=Tp;
        Tr+=Tp;
    }

    //Find next thread to run
    for(;;)
    {
        if(curInRound!=activeThreads.end()) curInRound++;
        if(curInRound==activeThreads.end()) //Note: do not replace with an else
        {
            //Check these two statements:
            //- If all threads are not ready, the scheduling algorithm must be
            //  paused and the idle thread is run instead
            //- If the inner integral regulator of all ready threads saturated
            //  then the integral regulator of the outer regulator must stop
            //  increasing because the set point cannot be attained anyway.
            bool allReadyThreadsSaturated=true;
            for(auto it=activeThreads.begin();it!=activeThreads.end();++it)
            {
                if((*it)->t->schedData.bo<bMax*multFactor)
                {
                    allReadyThreadsSaturated=false;
                    //Found a counterexample for both statements,
                    //no need to scan the list further.
                    break;
                }
            }
            if(activeThreads.empty())
            {
                //No thread is ready, run the idle thread

                //This is very important: the idle thread can *remove* dead
                //threads from threadList, so it can invalidate iterators
                //to any element except theadList.end()
                curInRound=activeThreads.end();
                runningThread=idle;
                ctxsave=runningThread->ctxsave;
                #ifdef WITH_PROCESSES
                MPUConfiguration::IRQdisable();
                #endif
                IRQsetNextPreemptionForIdle();
                #ifdef WITH_CPU_TIME_COUNTER
                IRQprofileContextSwitch(prev->timeCounterData,
                                        idle->timeCounterData,burstStart);
                #endif //WITH_CPU_TIME_COUNTER
                return;
            }

            //End of round reached, run scheduling algorithm
            curInRound=activeThreads.begin();
            IRQrunRegulator(allReadyThreadsSaturated);
        }

        if((*curInRound)->t->flags.isReady())
        {
            //Found a READY thread, so run this one
            runningThread=(*curInRound)->t;
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
            ctxsave=runningThread->ctxsave;
            #endif //WITH_PROCESSES
            IRQsetNextPreemption(runningThread->schedData.bo/multFactor);
            #ifdef WITH_CPU_TIME_COUNTER
            IRQprofileContextSwitch(prev->timeCounterData,
                                    (*curInRound)->t->timeCounterData,burstStart);
            #endif //WITH_CPU_TIME_COUNTER
            return;
        } else {
            //Error: a not ready thread end up in the ready list
            errorHandler(UNEXPECTED);
        }
    }
}

void ControlScheduler::IRQwaitStatusHook(Thread* t)
{
    // Managing activeThreads list
    if(t->flags.isReady() && !t->schedData.lastReadyStatus)
    {
        // The thread has became active -> put it in the list
        addThreadToActiveList(&t->schedData.atlEntry);
        t->schedData.lastReadyStatus=true;
    } else if(!t->flags.isReady() && t->schedData.lastReadyStatus) {
        // The thread is no longer active -> remove it from the list
        remThreadfromActiveList(&t->schedData.atlEntry);
        t->schedData.lastReadyStatus=false;
    }
    #ifdef ENABLE_FEEDFORWARD
    IRQrecalculateAlfa();
    #endif //ENABLE_FEEDFORWARD
}

void ControlScheduler::IRQrecalculateAlfa()
{
    //Sum of all priorities of all threads
    //Note that since priority goes from 0 to PRIORITY_MAX-1
    //but priorities we need go from 1 to PRIORITY_MAX we need to add one
    unsigned int sumPriority=0;
    for(auto it=activeThreads.begin();it!=activeThreads.end();++it)
    {
        #ifdef ENABLE_FEEDFORWARD
        //Count only ready threads
        if((*it)->t->flags.isReady())
            sumPriority+=(*it)->t->schedData.priority.get()+1;//Add one
        #else //ENABLE_FEEDFORWARD
        //Count all threads
        sumPriority+=(*it)->t->schedData.priority.get()+1;//Add one
        #endif //ENABLE_FEEDFORWARD
    }
    //This can happen when ENABLE_FEEDFORWARD is set and no thread is ready
    if(sumPriority==0) return;
    #ifndef SCHED_CONTROL_FIXED_POINT
    float base=1.0f/((float)sumPriority);
    for(Thread *it=threadList;it!=nullptr;it=it->schedData.next)
    {
        #ifdef ENABLE_FEEDFORWARD
        //Assign zero bursts to blocked threads
        if(it->flags.isReady())
        {
            it->schedData.alfa=base*((float)(it->schedData.priority.get()+1));
        } else {
            it->schedData.alfa=0;
        }
        #else //ENABLE_FEEDFORWARD
        //Assign bursts irrespective of thread blocking status
        it->schedData.alfa=base*((float)(it->schedData.priority.get()+1));
        #endif //ENABLE_FEEDFORWARD
    }
    #else //FIXED_POINT_MATH
    //Sum of all alfa is maximum value for an unsigned short
    unsigned int base=4096/sumPriority;
    for(Thread *it=threadList;it!=nullptr;it=it->schedData.next)
    {
        #ifdef ENABLE_FEEDFORWARD
        //Assign zero bursts to blocked threads
        if(it->flags.isReady())
        {
            it->schedData.alfa=base*(it->schedData.priority.get()+1);
        } else {
            it->schedData.alfa=0;
        }
        #else //ENABLE_FEEDFORWARD
        //Assign bursts irrespective of thread blocking status
        it->schedData.alfa=base*(it->schedData.priority.get()+1);
        #endif //ENABLE_FEEDFORWARD
    }
    #endif //FIXED_POINT_MATH
    reinitRegulator=true;
}

void ControlScheduler::IRQrunRegulator(bool allReadyThreadsSaturated)
{
    using namespace std;
    #ifdef SCHED_CONTROL_FIXED_POINT
    //The fixed point scheduler may overflow if Tr is higher than this
    Tr=min(Tr,524287);
    #endif //FIXED_POINT_MATH
    #ifdef ENABLE_REGULATOR_REINIT
    if(reinitRegulator==false)
    {
    #endif //ENABLE_REGULATOR_REINIT
        int eTr=SP_Tr-Tr;
        #ifndef SCHED_CONTROL_FIXED_POINT
        int bc=bco+static_cast<int>(krr*eTr-krr*zrr*eTro);
        #else //FIXED_POINT_MATH
        //Tr is clamped to 524287, so eTr uses at most 19bits. Considering
        //the 31bits of a signed int, we have 12bits free.
        const int fixedKrr=static_cast<int>(krr*2048);
        const int fixedKrrZrr=static_cast<int>(krr*zrr*1024);
        int bc=bco+(fixedKrr*eTr)/2048-(fixedKrrZrr*eTro)/1024;
        #endif //FIXED_POINT_MATH
        if(allReadyThreadsSaturated)
        {
            //If all inner regulators reached upper saturation,
            //allow only a decrease in the burst correction.
            if(bc<bco) bco=bc;
        } else bco=bc;

        bco=min<int>(max(bco,-Tr),bMax*threadListSize);
        #ifndef SCHED_CONTROL_FIXED_POINT
        float nextRoundTime=static_cast<float>(Tr+bco);
        #else //FIXED_POINT_MATH
        unsigned int nextRoundTime=Tr+bco; //Bounded to 20bits
        #endif //FIXED_POINT_MATH
        eTro=eTr;
        Tr=0;//Reset round time
        for(Thread *it=threadList;it!=nullptr;it=it->schedData.next)
        {
            //Recalculate per thread set point
            #ifndef SCHED_CONTROL_FIXED_POINT
            it->schedData.SP_Tp=static_cast<int>(
                    it->schedData.alfa*nextRoundTime);
            #else //FIXED_POINT_MATH
            //nextRoundTime is bounded to 20bits, alfa to 12bits,
            //so the multiplication fits in 32bits
            it->schedData.SP_Tp=(it->schedData.alfa*nextRoundTime)/4096;
            #endif //FIXED_POINT_MATH

            //Run each thread internal regulator
            int eTp=it->schedData.SP_Tp - it->schedData.Tp;
            //note: since b and bo contain the real value multiplied by
            //multFactor, this equals b=bo+eTp/multFactor.
            int b=it->schedData.bo + eTp;
            //saturation
            it->schedData.bo=min(max(b,bMin*multFactor),bMax*multFactor);
        }
    #ifdef ENABLE_REGULATOR_REINIT
    } else {
        reinitRegulator=false;
        Tr=0;//Reset round time
        //Reset state of the external regulator
        eTro=0;
        bco=0;

        for(Thread *it=threadList;it!=nullptr;it=it->schedData.next)
        {
            //Recalculate per thread set point
            #ifndef SCHED_CONTROL_FIXED_POINT
            it->schedData.SP_Tp=static_cast<int>(it->schedData.alfa*SP_Tr);
            #else //FIXED_POINT_MATH
            //SP_Tr is bounded to 20bits, alfa to 12bits,
            //so the multiplication fits in 32bits
            it->schedData.SP_Tp=(it->schedData.alfa*SP_Tr)/4096;
            #endif //FIXED_POINT_MATH

            int b=it->schedData.SP_Tp*multFactor;
            it->schedData.bo=min(max(b,bMin*multFactor),bMax*multFactor);
        }
    }
    #endif //ENABLE_REGULATOR_REINIT
}

Thread *ControlScheduler::threadList=nullptr;
unsigned int ControlScheduler::threadListSize=0;
Thread *ControlScheduler::idle=nullptr;
int ControlScheduler::SP_Tr=0;
int ControlScheduler::Tr=bNominal;
int ControlScheduler::bco=0;
int ControlScheduler::eTro=0;
bool ControlScheduler::reinitRegulator=false;

} //namespace miosix

#endif //SCHED_CONTROL_MULTIBURST
#endif //SCHED_TYPE_CONTROL_BASED
