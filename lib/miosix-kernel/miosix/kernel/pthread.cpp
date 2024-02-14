/***************************************************************************
 *   Copyright (C) 2010-2023 by Terraneo Federico                          *
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
 * pthread.cpp Part of the Miosix Embedded OS. Provides a mapping of the
 * posix thread API to the Miosix thread API.
 */

#include <sched.h>
#include <errno.h>
#include <stdexcept>
#include <algorithm>
#include "error.h"
#include "pthread_private.h"
#include "stdlib_integration/libc_integration.h"

using namespace miosix;

//
// Newlib's pthread.h has been patched since Miosix 1.68 to contain a definition
// for pthread_mutex_t and pthread_cond_t that allows a fast implementation
// of mutexes and condition variables. This *requires* to use an up-to-date gcc.
//

//These functions needs to be callable from C
extern "C" {

//
// Thread related API
//

int pthread_create(pthread_t *pthread, const pthread_attr_t *attr,
    void *(*start)(void *), void *arg)
{
    Thread::Options opt=Thread::JOINABLE;
    unsigned int stacksize=STACK_DEFAULT_FOR_PTHREAD;
    Priority priority=MAIN_PRIORITY;
    if(attr!=NULL)
    {
        if(attr->detachstate==PTHREAD_CREATE_DETACHED)
            opt=Thread::DEFAULT;
        stacksize=attr->stacksize;
        #ifndef SCHED_TYPE_EDF
        // Cap priority value in the range between 0 and PRIORITY_MAX-1
        int prio=std::min(std::max(0,attr->schedparam.sched_priority),
                          PRIORITY_MAX-1);
        // Swap unix-based priority back to the miosix one.
        priority=(PRIORITY_MAX-1)-prio;
        #endif //SCHED_TYPE_EDF
    }
    Thread *result=Thread::create(start,stacksize,priority,arg,opt);
    if(result==0) return EAGAIN;
    *pthread=reinterpret_cast<pthread_t>(result);
    return 0;
}

int pthread_join(pthread_t pthread, void **value_ptr)
{
    Thread *t=reinterpret_cast<Thread*>(pthread);
    if(Thread::exists(t)==false) return ESRCH;
    if(t==Thread::getCurrentThread()) return EDEADLK;
    if(t->join(value_ptr)==false) return EINVAL;
    return 0;
}

int pthread_detach(pthread_t pthread)
{
    Thread *t=reinterpret_cast<Thread*>(pthread);
    if(Thread::exists(t)==false) return ESRCH;
    t->detach();
    return 0;
}

pthread_t pthread_self()
{
    return reinterpret_cast<pthread_t>(Thread::getCurrentThread());
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
    return t1==t2;
}

int pthread_attr_init(pthread_attr_t *attr)
{
    //We only use three fields of pthread_attr_t so initialize only these
    attr->detachstate=PTHREAD_CREATE_JOINABLE;
    attr->stacksize=STACK_DEFAULT_FOR_PTHREAD;
    //Default priority level is one above minimum.
    #ifndef SCHED_TYPE_EDF
    attr->schedparam.sched_priority=PRIORITY_MAX-1-MAIN_PRIORITY;
    #endif //SCHED_TYPE_EDF
    return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr)
{
    return 0; //That was easy
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate)
{
    *detachstate=attr->detachstate;
    return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{
    if(detachstate!=PTHREAD_CREATE_JOINABLE &&
       detachstate!=PTHREAD_CREATE_DETACHED) return EINVAL;
    attr->detachstate=detachstate;
    return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize)
{
    *stacksize=attr->stacksize;
    return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
    if(stacksize<STACK_MIN) return EINVAL;
    attr->stacksize=stacksize;
    return 0;
}

int pthread_attr_getschedparam(const pthread_attr_t *attr,
                               struct sched_param *param)
{
    *param = attr->schedparam;
    return 0;
}

int pthread_attr_setschedparam(pthread_attr_t *attr,
                               const struct sched_param *param)
{
    attr->schedparam = *param;
    return 0;
}

#ifndef SCHED_TYPE_EDF
int sched_get_priority_max(int policy)
{
    (void)policy;

    // Unix-like thread priorities: max priority is zero.
    return 0;
}

int sched_get_priority_min(int policy)
{
    (void)policy;

    // Unix-like thread priorities: min priority is a value above zero.
    // The value for PRIORITY_MAX is configured in miosix_settings.h
    return PRIORITY_MAX - 1;
}
#endif //SCHED_TYPE_EDF

int sched_yield()
{
    Thread::yield();
    return 0;
}

//
// Mutex API
//

int	pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
    attr->recursive=PTHREAD_MUTEX_DEFAULT;
    return 0;
}

int	pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
    return 0; //Do nothing
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *kind)
{
    *kind=attr->recursive;
    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int kind)
{
    switch(kind)
    {
        case PTHREAD_MUTEX_DEFAULT:
            attr->recursive=PTHREAD_MUTEX_DEFAULT;
            return 0;
        case PTHREAD_MUTEX_RECURSIVE:
            attr->recursive=PTHREAD_MUTEX_RECURSIVE;
            return 0;
        default:
            return EINVAL;
    }
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    mutex->owner=0;
    mutex->first=0;
    //No need to initialize mutex->last
    if(attr!=0)
    {
        mutex->recursive= attr->recursive==PTHREAD_MUTEX_RECURSIVE ? 0 : -1;
    } else mutex->recursive=-1;
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    if(mutex->owner!=0) return EBUSY;
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    FastInterruptDisableLock dLock;
    IRQdoMutexLock(mutex,dLock);
    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    FastInterruptDisableLock dLock;
    void *p=reinterpret_cast<void*>(Thread::IRQgetCurrentThread());
    if(mutex->owner==0)
    {
        mutex->owner=p;
        return 0;
    }
    if(mutex->owner==p && mutex->recursive>=0)
    {
        mutex->recursive++;
        return 0;
    }
    return EBUSY;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    #ifndef SCHED_TYPE_EDF
    FastInterruptDisableLock dLock;
    IRQdoMutexUnlock(mutex);
    #else //SCHED_TYPE_EDF
    bool hppw;
    {
        FastInterruptDisableLock dLock;
        hppw=IRQdoMutexUnlock(mutex);
    }
    if(hppw) Thread::yield(); //If the woken thread has higher priority, yield
    #endif //SCHED_TYPE_EDF
    return 0;
}

//
// Condition variable API
//

//The pthread_cond_t API is implemented simply as a wrapper around the native
//Miosix C++ ConditionVariable. Therefore the memory layout of pthread_cond_t
//and of ConditionVariable must be exactly the same.

static_assert(sizeof(ConditionVariable)==sizeof(pthread_cond_t),"Invalid pthread_cond_t size");

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    //attr is currently not considered
    //NOTE: pthread_condattr_setclock is not supported, the only clock supported
    //for pthread_cond_timedwait is CLOCK_MONOTONIC
    new (cond) ConditionVariable; //Placement new as cond is a C type
    return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
    auto *impl=reinterpret_cast<ConditionVariable*>(cond);
    if(!impl->condList.empty()) return EBUSY;
    impl->~ConditionVariable(); //Call destructor manually
    return 0;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    auto *impl=reinterpret_cast<ConditionVariable*>(cond);
    impl->wait(mutex);
    return 0;
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
    auto *impl=reinterpret_cast<ConditionVariable*>(cond);
    TimedWaitResult res = impl->timedWait(mutex,timespec2ll(abstime));
    return res == TimedWaitResult::Timeout ? ETIMEDOUT : 0;
}

int pthread_cond_signal(pthread_cond_t *cond)
{
    auto *impl=reinterpret_cast<ConditionVariable*>(cond);
    bool hppw=impl->doSignal();
    (void)hppw;
    /*
     * A note on the conditional yield. Doing a pthread_cond_signal/broadcast is
     * permitted either with the mutex locked or not. If we're calling signal
     * with the mutex locked, yielding isn't a good idea even if we woke up a
     * higher priority thread as we "bounce back" since the woken thread will
     * block trying to lock the mutex we're holding. Also, the mutex unlock
     * will yield anyway and immediately switch to the higher priority thread.
     * The issue is, within signal/broadcast, we don't know if we're being
     * called with the mutex locked or not.
     * So, we implement the following design choice:
     * - in the ConditionVariable class we unconditionally yield. Since
     *   ConditionVariable can also be used with priority inheritance mutexes,
     *   we assume that we really care about minimizing latency in waking up
     *   higher priority threads, and we take the bounce back overhead if the
     *   signal was called with the mutex locked.
     * - in pthread_cond_singal/broadcast we don't yield since they can only be
     *   used with non-priority-inheritance-mutexes, and if the signal is
     *   called with no mutex locked, the actual context switch to the higher
     *   priority thread will be delayed to the next preemption. Except with
     *   EDF though, as it does not preempt after a time quantum so the delay
     *   could be indefinitely long. In that case, we always yield.
     */
    #ifdef SCHED_TYPE_EDF
    //If the woken thread has higher priority than our priority, yield
    if(hppw) Thread::yield();
    #endif //SCHED_TYPE_EDF
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
    auto *impl=reinterpret_cast<ConditionVariable*>(cond);
    bool hppw=impl->doBroadcast();
    (void)hppw;
    #ifdef SCHED_TYPE_EDF
    //If at least one woken thread has higher priority than our priority, yield
    if(hppw) Thread::yield();
    #endif //SCHED_TYPE_EDF
    return 0;
}

//
// Once API
//

int pthread_once(pthread_once_t *once, void (*func)())
{
    if(once==nullptr || func==nullptr || once->is_initialized!=1) return EINVAL;

    bool again;
    do {
        {
            FastInterruptDisableLock dLock;
            switch(once->init_executed)
            {
                case 0: //We're the first ones (or previous call has thrown)
                    once->init_executed=1;
                    again=false;
                    break;
                case 1: //Call started but not ended
                    again=true;
                    break;
                default: //Already called, return immediately
                    return 0;
            }
        }
        #ifndef SCHED_TYPE_EDF
        if(again) Thread::yield(); //Yield and let other thread complete
        #else //SCHED_TYPE_EDF
        if(again) Thread::sleep(1); //Can't yield with EDF, this may be slow
        #endif //SCHED_TYPE_EDF
    } while(again);

    #ifdef __NO_EXCEPTIONS
    func();
    #else //__NO_EXCEPTIONS
    try {
        func();
    } catch(...) {
        once->init_executed=0; //We failed, let some other thread try
        throw;
    }
    #endif //__NO_EXCEPTIONS
    once->init_executed=2; //We succeeded
    return 0;
}

int pthread_setcancelstate(int state, int *oldstate) { return 0; } //Stub

} //extern "C"

