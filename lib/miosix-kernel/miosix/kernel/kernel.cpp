/***************************************************************************
 *   Copyright (C) 2008-2023 by Terraneo Federico                          *
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

#include "kernel.h"
#include "interfaces/portability.h"
#include "interfaces/atomic_ops.h"
#include "error.h"
#include "logging.h"
#include "sync.h"
#include "stage_2_boot.h"
#include "process.h"
#include "kernel/scheduler/scheduler.h"
#include "stdlib_integration/libc_integration.h"
#include "interfaces/os_timer.h"
#include "timeconversion.h"
#include <stdexcept>
#include <algorithm>
#include <limits>
#include <string.h>
#include <reent.h>
#include "interfaces/deep_sleep.h"

/*
 * Used by assembler context switch macros
 * This variable is set by miosix::IRQfindNextThread in file kernel.cpp
 */
extern "C" {
volatile unsigned int *ctxsave;
}


namespace miosix {

//Global variables used by kernel.cpp. Those that are not static are also used
//in portability.cpp and by the schedulers.
//These variables MUST NOT be used outside kernel.cpp and portability.cpp

volatile Thread *runningThread=nullptr;///<\internal Thread currently running

///\internal True if there are threads in the DELETED status. Used by idle thread
static volatile bool existDeleted=false;

IntrusiveList<SleepData> sleepingList;///list of sleeping threads

///\internal !=0 after pauseKernel(), ==0 after restartKernel()
volatile int kernelRunning=0;

///\internal true if a thread wakeup occurs while the kernel is paused
volatile bool pendingWakeup=false;

static bool kernelStarted=false;///<\internal becomes true after startKernel.

/// This is used by disableInterrupts() and enableInterrupts() to allow nested
/// calls to these functions.
static unsigned char interruptDisableNesting=0;

#ifdef WITH_DEEP_SLEEP

///  This variable is used to keep count of how many peripherals are actually used.
/// If it 0 then the system can enter the deep sleep state
static int deepSleepCounter=0;

#endif // WITH_DEEP_SLEEP

#ifdef WITH_PROCESSES

/// The proc field of the Thread class for kernel threads points to this object
static ProcessBase *kernel=nullptr;

#endif //WITH_PROCESSES

/**
 * \internal
 * Idle thread. Created when the kernel is started, it phisically deallocates
 * memory for deleted threads, and puts the cpu in sleep mode.
 */
void *idleThread(void *argv)
{
    (void)argv;

    for(;;)
    {
        if(existDeleted)
        {
            PauseKernelLock lock;
            existDeleted=false;
            Scheduler::PKremoveDeadThreads();
        }
        #ifndef JTAG_DISABLE_SLEEP
        //JTAG debuggers lose communication with the device if it enters sleep
        //mode, so to use debugging it is necessary to remove this instruction
        
        #ifdef WITH_DEEP_SLEEP
        {
            FastInterruptDisableLock lock;
            bool sleep;
            if(deepSleepCounter==0)
            {
                if(sleepingList.empty()==false)
                {
                    long long wakeup=sleepingList.front()->wakeupTime;
                    sleep=!IRQdeepSleep(wakeup);
                } else sleep=!IRQdeepSleep();
            } else sleep=true;
            //NOTE: going to sleep with interrupts disabled makes sure no
            //preemption occurs from when we take the decision to sleep till
            //we actually do sleep. Wakeup interrupt will be run when we enable
            //back interrupts
            if(sleep) miosix_private::sleepCpu();
        }
        #else //WITH_DEEP_SLEEP
        miosix_private::sleepCpu();
        #endif //WITH_DEEP_SLEEP
        
        #endif //JTAG_DISABLE_SLEEP
    }
    return 0; //Just to avoid a compiler warning
}

void disableInterrupts()
{
    //Before the kernel is started interrupts are disabled,
    //so disabling them again won't hurt
    miosix_private::doDisableInterrupts();
    if(interruptDisableNesting==0xff) errorHandler(NESTING_OVERFLOW);
    interruptDisableNesting++;
}

void enableInterrupts()
{
    if(interruptDisableNesting==0)
    {
        //Bad, enableInterrupts was called one time more than disableInterrupts
        errorHandler(DISABLE_INTERRUPTS_NESTING);
    }
    interruptDisableNesting--;
    if(interruptDisableNesting==0 && kernelStarted==true)
    {
        miosix_private::doEnableInterrupts();
    }
}

void pauseKernel()
{
    int old=atomicAddExchange(&kernelRunning,1);
    if(old>=0xff) errorHandler(NESTING_OVERFLOW);
}

void restartKernel()
{
    int old=atomicAddExchange(&kernelRunning,-1);
    if(old<=0) errorHandler(PAUSE_KERNEL_NESTING);
    
    //Check interruptDisableNesting to allow pauseKernel() while interrupts
    //are disabled with an InterruptDisableLock
    if(interruptDisableNesting==0)
    {
        //If we missed preemptions yield immediately
        if(old==1 && pendingWakeup)
        { 
            pendingWakeup=false;
            Thread::yield();
        }
    }
}

bool areInterruptsEnabled()
{
    return miosix_private::checkAreInterruptsEnabled();
}

void deepSleepLock()
{
    #ifdef WITH_DEEP_SLEEP
    atomicAdd(&deepSleepCounter,1);
    #endif // WITH_DEEP_SLEEP
}

void deepSleepUnlock()
{
    #ifdef WITH_DEEP_SLEEP
    atomicAdd(&deepSleepCounter,-1);
    #endif // WITH_DEEP_SLEEP
}

void startKernel()
{
    #ifdef WITH_PROCESSES
    try {
        kernel=new ProcessBase;
    } catch(...) {
        errorHandler(OUT_OF_MEMORY);
    }
    #endif //WITH_PROCESSES
    
    // As a side effect this function allocates the idle thread and makes
    // runningThread point to it. It's probably been called many times during
    // boot by the time we get here, but we can't be sure
    auto *idle=Thread::IRQgetCurrentThread();
    
    #ifdef WITH_PROCESSES
    // If the idle thread was allocated before startKernel(), then its proc
    // is nullptr. We can't move kernel=new ProcessBase; earlier than this
    // function, though
    idle->proc=kernel;
    #endif //WITH_PROCESSES

    // Create the idle and main thread
    Thread *main;
    main=Thread::doCreate(mainLoader,MAIN_STACK_SIZE,nullptr,Thread::DEFAULT,true);
    if(main==nullptr) errorHandler(OUT_OF_MEMORY);
    
    // Add them to the scheduler
    if(Scheduler::PKaddThread(main,MAIN_PRIORITY)==false) errorHandler(UNEXPECTED);

    // Idle thread needs to be set after main (see control_scheduler.cpp)
    Scheduler::IRQsetIdleThread(idle);
    
    // Make the C standard library use per-thread reeentrancy structure
    setCReentrancyCallback(Thread::getCReent);
    
    // Dispatch the task to the architecture-specific function
    kernelStarted=true;
    miosix_private::IRQportableStartKernel();
}

bool isKernelRunning()
{
    return (kernelRunning==0) && kernelStarted;
}

//These are not implemented here, but in the platform/board-specific os_timer.
//long long getTime() noexcept
//long long IRQgetTime() noexcept

/**
 * \internal
 * Used by Thread::sleep() and pthread_cond_timedwait() to add a thread to
 * sleeping list. The list is sorted by the wakeupTime field to reduce time
 * required to wake threads during context switch.
 * Interrupts must be disabled prior to calling this function.
 */
static void IRQaddToSleepingList(SleepData *x)
{
    if(sleepingList.empty() || sleepingList.front()->wakeupTime>=x->wakeupTime)
    {
        sleepingList.push_front(x);
    } else {
        auto it=sleepingList.begin();
        while(it!=sleepingList.end() && (*it)->wakeupTime<x->wakeupTime) ++it;
        sleepingList.insert(it,x);
    }
}

/**
 * \internal
 * Called to check if it's time to wake some thread.
 * Takes care of clearing SLEEP_FLAG.
 * It is used by the kernel, and should not be used by end users.
 * \return true if some thread with higher priority of current thread is woken.
 */
bool IRQwakeThreads(long long currentTime)
{
    if(sleepingList.empty()) return false; //If no item in list, return
    
    bool result=false;
    //Since list is sorted, if we don't need to wake the first element
    //we don't need to wake the other too
    for(auto it=sleepingList.begin();it!=sleepingList.end();)
    {
        if(currentTime<(*it)->wakeupTime) break;
        //Wake both threads doing absoluteSleep() and timedWait()
        (*it)->thread->flags.IRQclearSleepAndWait();
        if(const_cast<Thread*>(runningThread)->IRQgetPriority()<(*it)->thread->IRQgetPriority())
            result=true;
        it=sleepingList.erase(it);
    }
    return result;
}

/*
Memory layout for a thread
    |------------------------|
    |     class Thread       |
    |------------------------|<-- this
    |         stack          |
    |           |            |
    |           V            |
    |------------------------|
    |       watermark        |
    |------------------------|<-- base, watermark
*/

Thread *Thread::create(void *(*startfunc)(void *), unsigned int stacksize,
                       Priority priority, void *argv, unsigned short options)
{
    //Check to see if input parameters are valid
    if(priority.validate()==false || stacksize<STACK_MIN) return nullptr;
    
    Thread *thread=doCreate(startfunc,stacksize,argv,options,false);
    if(thread==nullptr) return nullptr;
    
    //Add thread to thread list
    {
        //Handling the list of threads, critical section is required
        PauseKernelLock lock;
        if(Scheduler::PKaddThread(thread,priority)==false)
        {
            //Reached limit on number of threads
            unsigned int *base=thread->watermark;
            thread->~Thread();
            free(base); //Delete ALL thread memory
            return nullptr;
        }
    }
    #ifdef SCHED_TYPE_EDF
    if(isKernelRunning()) yield(); //The new thread might have a closer deadline
    #endif //SCHED_TYPE_EDF
    return thread;
}

Thread *Thread::create(void (*startfunc)(void *), unsigned int stacksize,
                       Priority priority, void *argv, unsigned short options)
{
    //Just call the other version with a cast.
    return Thread::create(reinterpret_cast<void (*)(void*)>(startfunc),
            stacksize,priority,argv,options);
}

void Thread::yield()
{
    miosix_private::doYield();
}

void Thread::sleep(unsigned int ms)
{
    nanoSleepUntil(getTime()+mul32x32to64(ms,1000000));
}

void Thread::nanoSleep(long long ns)
{
    nanoSleepUntil(getTime()+ns);
}

void Thread::nanoSleepUntil(long long absoluteTimeNs)
{
    //Disallow absolute sleeps with negative or too low values, as the ns2tick()
    //algorithm in TimeConversion can't handle negative values and may undeflow
    //even with very low values due to a negative adjustOffsetNs. As an unlikely
    //side effect, very short sleeps done very early at boot will be extended.
    absoluteTimeNs=std::max(absoluteTimeNs,100000LL);
    //pauseKernel() here is not enough since even if the kernel is stopped
    //the timer isr will wake threads, modifying the sleepingList
    {
        FastInterruptDisableLock dLock;
        SleepData d(const_cast<Thread*>(runningThread),absoluteTimeNs);
        d.thread->flags.IRQsetSleep(); //Sleeping thread: set sleep flag
        IRQaddToSleepingList(&d);
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::yield();
        }
        //Only required for interruptibility when terminate is called
        sleepingList.removeFast(&d);
    }
}

void Thread::wait()
{
    //pausing the kernel is not enough because of IRQwait and IRQwakeup
    {
        FastInterruptDisableLock lock;
        const_cast<Thread*>(runningThread)->flags.IRQsetWait(true);
    }
    Thread::yield();
    //Return here after wakeup
}

void Thread::IRQwait()
{
    const_cast<Thread*>(runningThread)->flags.IRQsetWait(true);
}

void Thread::PKrestartKernelAndWait(PauseKernelLock& dLock)
{
    (void)dLock;
    //Implemented by upgrading the lock to an interrupt disable one
    FastInterruptDisableLock dLockIrq;
    auto savedNesting=kernelRunning;
    kernelRunning=0;
    IRQenableIrqAndWaitImpl();
    if(kernelRunning!=0) errorHandler(UNEXPECTED);
    kernelRunning=savedNesting;
}

TimedWaitResult Thread::PKrestartKernelAndTimedWait(PauseKernelLock& dLock,
        long long absoluteTimeNs)
{
    (void)dLock;
    //Implemented by upgrading the lock to an interrupt disable one
    FastInterruptDisableLock dLockIrq;
    auto savedNesting=kernelRunning;
    kernelRunning=0;
    auto result=IRQenableIrqAndTimedWaitImpl(absoluteTimeNs);
    if(kernelRunning!=0) errorHandler(UNEXPECTED);
    kernelRunning=savedNesting;
    return result;
}

void Thread::wakeup()
{
    //pausing the kernel is not enough because of IRQwait and IRQwakeup
    {
        FastInterruptDisableLock lock;
        this->flags.IRQsetWait(false);
    }
    #ifdef SCHED_TYPE_EDF
    yield();//The other thread might have a closer deadline
    #endif //SCHED_TYPE_EDF
}

void Thread::PKwakeup()
{
    //pausing the kernel is not enough because of IRQwait and IRQwakeup
    FastInterruptDisableLock lock;
    this->flags.IRQsetWait(false);
}

void Thread::IRQwakeup()
{
    this->flags.IRQsetWait(false);
}

Thread *Thread::IRQgetCurrentThread()
{
    //NOTE: this code is currently safe to be called either with interrupt
    //enabed or not, and with the kernel paused or not, as well as before the
    //kernel is started, so getCurrentThread() and PKgetCurrentThread() all
    //directly call here. If introducing changes that break this property, these
    //three functions may need to be split

    //Implementation is the same as getCurrentThread, but to keep a consistent
    //interface this method is duplicated
    Thread *result=const_cast<Thread*>(runningThread);
    if(result) return result;
    //This function must always return a pointer to a valid thread. The first
    //time this is called before the kernel is started, however, runningThread
    //is nullptr, thus we allocate the idle thread and return a pointer to that.
    return allocateIdleThread();
}

bool Thread::exists(Thread *p)
{
    if(p==nullptr) return false;
    PauseKernelLock lock;
    return Scheduler::PKexists(p);
}

Priority Thread::getPriority()
{
    //NOTE: the code in all schedulers is currently safe to be called either
    //with interrupt enabed or not, and with the kernel paused or not, so
    //PKgetPriority() and IRQgetPriority() directly call here. If introducing
    //changes that break this property, these functions may need to be split
    return Scheduler::getPriority(this);
}

void Thread::setPriority(Priority pr)
{
    if(pr.validate()==false) return;
    PauseKernelLock lock;

    Thread *running=PKgetCurrentThread();
    //If thread is locking at least one mutex
    if(running->mutexLocked!=nullptr)
    {   
        //savedPriority always changes, since when all mutexes are unlocked
        //setPriority() must become effective
        if(running->savedPriority==pr) return;
        running->savedPriority=pr;
        //Calculate new priority of thread, which is
        //max(savedPriority, inheritedPriority)
        Mutex *walk=running->mutexLocked;
        while(walk!=nullptr)
        {
            if(walk->waiting.empty()==false)
                pr=std::max(pr,walk->waiting.front()->PKgetPriority());
            walk=walk->next;
        }
    }
    
    //If old priority == desired priority, nothing to do.
    if(pr==running->PKgetPriority()) return;
    Scheduler::PKsetPriority(running,pr);
    #ifdef SCHED_TYPE_EDF
    if(isKernelRunning()) yield(); //Another thread might have a closer deadline
    #endif //SCHED_TYPE_EDF
}

void Thread::terminate()
{
    //doing a read-modify-write operation on this->status, so pauseKernel is
    //not enough, we need to disable interrupts
    FastInterruptDisableLock lock;
    if(this->flags.isDeleting()) return; //Prevent sleep interruption abuse
    this->flags.IRQsetDeleting();
    this->flags.IRQclearSleepAndWait(); //Interruptibility
}

bool Thread::testTerminate()
{
    //Just reading, no need for critical section
    return const_cast<Thread*>(runningThread)->flags.isDeleting();
}

void Thread::detach()
{
    FastInterruptDisableLock lock;
    this->flags.IRQsetDetached();
    
    //we detached a terminated thread, so its memory needs to be deallocated
    if(this->flags.isDeletedJoin()) existDeleted=true;

    //Corner case: detaching a thread, but somebody else already called join
    //on it. This makes join return false instead of deadlocking
    if(this->joinData.waitingForJoin!=nullptr)
    {
        //joinData is an union, so its content can be an invalid thread
        //this happens if detaching a thread that has already terminated
        if(this->flags.isDeletedJoin()==false)
        {
            //Wake thread, or it might sleep forever
            this->joinData.waitingForJoin->flags.IRQsetJoinWait(false);
        }
    }
}

bool Thread::isDetached() const
{
    return this->flags.isDetached();
}

bool Thread::join(void** result)
{
    {
        FastInterruptDisableLock dLock;
        if(this==Thread::IRQgetCurrentThread()) return false;
        if(Thread::IRQexists(this)==false) return false;
        if(this->flags.isDetached()) return false;
        if(this->flags.isDeletedJoin()==false)
        {
            //Another thread already called join on toJoin
            if(this->joinData.waitingForJoin!=nullptr) return false;

            this->joinData.waitingForJoin=Thread::IRQgetCurrentThread();
            for(;;)
            {
                //Wait
                Thread::IRQgetCurrentThread()->flags.IRQsetJoinWait(true);
                {
                    FastInterruptEnableLock eLock(dLock);
                    Thread::yield();
                }
                if(Thread::IRQexists(this)==false) return false;
                if(this->flags.isDetached()) return false;
                if(this->flags.isDeletedJoin()) break;
            }
        }
        //Thread deleted, complete join procedure
        //Setting detached flag will make isDeleted() return true,
        //so its memory can be deallocated
        this->flags.IRQsetDetached();
        if(result!=nullptr) *result=this->joinData.result;
    }
    {
        PauseKernelLock lock;
        //Since there is surely one dead thread, deallocate it immediately
        //to free its memory as soon as possible
        Scheduler::PKremoveDeadThreads();
    }
    return true;
}

const unsigned int *Thread::getStackBottom()
{
    return getCurrentThread()->watermark+(WATERMARK_LEN/sizeof(unsigned int));
}

int Thread::getStackSize()
{
    return getCurrentThread()->stacksize;
}

void Thread::IRQstackOverflowCheck()
{
    const unsigned int watermarkSize=WATERMARK_LEN/sizeof(unsigned int);
    for(unsigned int i=0;i<watermarkSize;i++)
    {
        if(runningThread->watermark[i]!=WATERMARK_FILL) errorHandler(STACK_OVERFLOW);
    }
    if(runningThread->ctxsave[stackPtrOffsetInCtxsave] <
        reinterpret_cast<unsigned int>(runningThread->watermark+watermarkSize))
        errorHandler(STACK_OVERFLOW);
}

#ifdef WITH_PROCESSES

void Thread::IRQhandleSvc(unsigned int svcNumber)
{
    if(runningThread->proc==kernel) errorHandler(UNEXPECTED);
    if(svcNumber==SYS_USERSPACE)
    {
        const_cast<Thread*>(runningThread)->flags.IRQsetUserspace(true);
        ::ctxsave=runningThread->userCtxsave;
        //We know it's not the kernel, so the cast is safe
        static_cast<Process*>(runningThread->proc)->mpu.IRQenable();
    } else {
        const_cast<Thread*>(runningThread)->flags.IRQsetUserspace(false);
        ::ctxsave=runningThread->ctxsave;
        MPUConfiguration::IRQdisable();
    }
}

bool Thread::IRQreportFault(const miosix_private::FaultData& fault)
{
    if(const_cast<Thread*>(runningThread)->flags.isInUserspace()==false
        || runningThread->proc==kernel) return false;
    //We know it's not the kernel, so the cast is safe
    static_cast<Process*>(runningThread->proc)->fault=fault;
    const_cast<Thread*>(runningThread)->flags.IRQsetUserspace(false);
    ::ctxsave=runningThread->ctxsave;
    MPUConfiguration::IRQdisable();
    return true;
}

miosix_private::SyscallParameters Thread::switchToUserspace()
{
    miosix_private::portableSwitchToUserspace();
    miosix_private::SyscallParameters result(runningThread->userCtxsave);
    return result;
}

Thread *Thread::createUserspace(void *(*startfunc)(void *), void *argv,
                    unsigned short options, Process *proc)
{
    Thread *thread=doCreate(startfunc,SYSTEM_MODE_PROCESS_STACK_SIZE,argv,
            options,false);
    if(thread==nullptr) return nullptr;

    unsigned int *base=thread->watermark;
    try {
        thread->userCtxsave=new unsigned int[CTXSAVE_SIZE];
    } catch(std::bad_alloc&) {
        thread->~Thread();
        free(base); //Delete ALL thread memory
        return nullptr;//Error
    }
    
    thread->proc=proc;
    thread->flags.IRQsetWait(true); //Thread is not yet ready
    
    //Add thread to thread list
    {
        //Handling the list of threads, critical section is required
        PauseKernelLock lock;
        if(Scheduler::PKaddThread(thread,MAIN_PRIORITY)==false)
        {
            //Reached limit on number of threads
            base=thread->watermark;
            thread->~Thread();
            free(base); //Delete ALL thread memory
            return nullptr;
        }
    }

    return thread;
}

void Thread::setupUserspaceContext(unsigned int entry, unsigned int *gotBase,
    unsigned int ramImageSize)
{
    void *(*startfunc)(void*)=reinterpret_cast<void *(*)(void*)>(entry);
    unsigned int *ep=gotBase+ramImageSize/sizeof(int);
    miosix_private::initCtxsave(runningThread->userCtxsave,startfunc,ep,nullptr,gotBase);
}

#endif //WITH_PROCESSES

Thread::Thread(unsigned int *watermark, unsigned int stacksize,
               bool defaultReent) : schedData(), flags(this), savedPriority(0),
               mutexLocked(nullptr), mutexWaiting(nullptr), watermark(watermark),
               ctxsave(), stacksize(stacksize)
{
    joinData.waitingForJoin=nullptr;
    if(defaultReent) cReentrancyData=_GLOBAL_REENT;
    else {
        cReentrancyData=new _reent;
        if(cReentrancyData) _REENT_INIT_PTR(cReentrancyData);
    }
    #ifdef WITH_PROCESSES
    proc=kernel;
    userCtxsave=nullptr;
    #endif //WITH_PROCESSES
}

Thread::~Thread()
{
    if(cReentrancyData && cReentrancyData!=_GLOBAL_REENT)
    {
        _reclaim_reent(cReentrancyData);
        delete cReentrancyData;
    }
    #ifdef WITH_PROCESSES
    if(userCtxsave) delete[] userCtxsave;
    #endif //WITH_PROCESSES
}

Thread *Thread::doCreate(void*(*startfunc)(void*) , unsigned int stacksize,
                      void* argv, unsigned short options, bool defaultReent)
{
    unsigned int fullStackSize=WATERMARK_LEN+CTXSAVE_ON_STACK+stacksize;

    //Align fullStackSize to the platform required stack alignment
    fullStackSize+=CTXSAVE_STACK_ALIGNMENT-1;
    fullStackSize/=CTXSAVE_STACK_ALIGNMENT;
    fullStackSize*=CTXSAVE_STACK_ALIGNMENT;

    //Allocate memory for the thread, return if fail
    unsigned int *base=static_cast<unsigned int*>(malloc(sizeof(Thread)+
            fullStackSize));
    if(base==nullptr) return nullptr;

    //At the top of thread memory allocate the Thread class with placement new
    void *threadClass=base+(fullStackSize/sizeof(unsigned int));
    Thread *thread=new (threadClass) Thread(base,stacksize,defaultReent);

    if(thread->cReentrancyData==nullptr)
    {
         thread->~Thread();
         free(base); //Delete ALL thread memory
         return nullptr;
    }

    //Fill watermark and stack
    memset(base, WATERMARK_FILL, WATERMARK_LEN);
    base+=WATERMARK_LEN/sizeof(unsigned int);
    memset(base, STACK_FILL, fullStackSize-WATERMARK_LEN);

    //On some architectures some registers are saved on the stack, therefore
    //initCtxsave *must* be called after filling the stack.
    miosix_private::initCtxsave(thread->ctxsave,startfunc,
            reinterpret_cast<unsigned int*>(thread),argv);

    if((options & JOINABLE)==0) thread->flags.IRQsetDetached();
    return thread;
}

void Thread::threadLauncher(void *(*threadfunc)(void*), void *argv)
{
    void *result=nullptr;
    #ifdef __NO_EXCEPTIONS
    result=threadfunc(argv);
    #else //__NO_EXCEPTIONS
    try {
        result=threadfunc(argv);
    } catch(std::exception& e) {
        errorLog("***An exception propagated through a thread\n");
        errorLog("what():%s\n",e.what());
    } catch(...) {
        errorLog("***An exception propagated through a thread\n");
    }
    #endif //__NO_EXCEPTIONS
    //Thread returned from its entry point, so delete it

    //Since the thread is running, it cannot be in the sleepingList, so no need
    //to remove it from the list
    {
        FastInterruptDisableLock lock;
        const_cast<Thread*>(runningThread)->flags.IRQsetDeleted();

        if(const_cast<Thread*>(runningThread)->flags.isDetached()==false)
        {
            //If thread is joinable, handle join
            if(runningThread->joinData.waitingForJoin!=nullptr)
            {
                //Wake thread
                runningThread->joinData.waitingForJoin->flags.IRQsetJoinWait(false);
            }
            //Set result
            runningThread->joinData.result=result;
        } else {
            //If thread is detached, memory can be deallocated immediately
            existDeleted=true;
        }
    }
    Thread::yield();//Since the thread is now deleted, yield immediately.
    //Will never reach here
    errorHandler(UNEXPECTED);
}

void Thread::IRQenableIrqAndWaitImpl()
{
    const_cast<Thread*>(runningThread)->flags.IRQsetWait(true);
    auto savedNesting=interruptDisableNesting; //For InterruptDisableLock
    interruptDisableNesting=0;
    miosix_private::doEnableInterrupts();
    Thread::yield(); //Here the wait becomes effective
    miosix_private::doDisableInterrupts();
    if(interruptDisableNesting!=0) errorHandler(UNEXPECTED);
    interruptDisableNesting=savedNesting;
}

TimedWaitResult Thread::IRQenableIrqAndTimedWaitImpl(long long absoluteTimeNs)
{
    absoluteTimeNs=std::max(absoluteTimeNs,100000LL);
    Thread *t=const_cast<Thread*>(runningThread);
    SleepData sleepData(t,absoluteTimeNs);
    t->flags.IRQsetWait(true); //timedWait thread: set wait flag
    IRQaddToSleepingList(&sleepData);
    auto savedNesting=interruptDisableNesting; //For InterruptDisableLock
    interruptDisableNesting=0;
    miosix_private::doEnableInterrupts();
    Thread::yield(); //Here the wait becomes effective
    miosix_private::doDisableInterrupts();
    if(interruptDisableNesting!=0) errorHandler(UNEXPECTED);
    interruptDisableNesting=savedNesting;
    bool removed=sleepingList.removeFast(&sleepData);
    //If the thread was still in the sleeping list, it was woken up by a wakeup()
    return removed ? TimedWaitResult::NoTimeout : TimedWaitResult::Timeout;
}

bool Thread::IRQexists(Thread* p)
{
    if(p==nullptr) return false;
    //NOTE: the code in all schedulers is currently safe to be called also with
    //interrupts disabled
    return Scheduler::PKexists(p);
}

Thread *Thread::allocateIdleThread()
{
    //NOTE: this function is only called once before the kernel is started, so
    //there are no concurrency issues, not even with interrupts

    // Create the idle and main thread
    auto *idle=Thread::doCreate(idleThread,STACK_IDLE,nullptr,Thread::DEFAULT,true);
    if(idle==nullptr) errorHandler(OUT_OF_MEMORY);

    // runningThread must point to a valid thread, so we make it point to the the idle one
    runningThread=idle;
    return idle;
}

struct _reent *Thread::getCReent()
{
    return getCurrentThread()->cReentrancyData;
}

//
// class ThreadFlags
//

void Thread::ThreadFlags::IRQsetWait(bool waiting)
{
    if(waiting) flags |= WAIT; else flags &= ~WAIT;
    Scheduler::IRQwaitStatusHook(this->t);
}

void Thread::ThreadFlags::IRQsetSleep()
{
    flags |= SLEEP;
    Scheduler::IRQwaitStatusHook(this->t);
}

void Thread::ThreadFlags::IRQclearSleepAndWait()
{
    flags &= ~(WAIT | SLEEP);
    Scheduler::IRQwaitStatusHook(this->t);
}

void Thread::ThreadFlags::IRQsetJoinWait(bool waiting)
{
    if(waiting) flags |= WAIT_JOIN; else flags &= ~WAIT_JOIN;
    Scheduler::IRQwaitStatusHook(this->t);
}

void Thread::ThreadFlags::IRQsetDeleted()
{
    flags |= DELETED;
    Scheduler::IRQwaitStatusHook(this->t);
}

} //namespace miosix
