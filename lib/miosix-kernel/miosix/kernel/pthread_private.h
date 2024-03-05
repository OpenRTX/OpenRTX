/***************************************************************************
 *   Copyright (C) 2011-2023 by Terraneo Federico                          *
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

//This file contains private implementation details of mutexes, it's not
//meant to be used by end users

#pragma once

#include <pthread.h>
#include "kernel.h"
#include "intrusive.h"
#include "sync.h"

namespace miosix {

/**
 * \internal
 * Implementation code to lock a mutex. Must be called with interrupts disabled
 * \param mutex mutex to be locked
 * \param d The instance of FastInterruptDisableLock used to disable interrupts
 */
static inline void IRQdoMutexLock(pthread_mutex_t *mutex,
        FastInterruptDisableLock& d)
{
    void *p=reinterpret_cast<void*>(Thread::IRQgetCurrentThread());
    if(mutex->owner==nullptr)
    {
        mutex->owner=p;
        return;
    }

    //This check is very important. Without this attempting to lock the same
    //mutex twice won't cause a deadlock because the wait is enclosed in a
    //while(owner!=p) which is immeditely false.
    if(mutex->owner==p)
    {
        if(mutex->recursive>=0)
        {
            mutex->recursive++;
            return;
        } else errorHandler(MUTEX_DEADLOCK); //Bad, deadlock
    }

    WaitingList waiting; //Element of a linked list on stack
    waiting.thread=p;
    waiting.next=nullptr; //Putting this thread last on the list (lifo policy)
    if(mutex->first==nullptr)
    {
        mutex->first=&waiting;
        mutex->last=&waiting;
    } else {
        mutex->last->next=&waiting;
        mutex->last=&waiting;
    }

    //The while is necessary to protect against spurious wakeups
    while(mutex->owner!=p) Thread::IRQenableIrqAndWait(d);
}

/**
 * \internal
 * Implementation code to lock a mutex to a specified depth level.
 * Must be called with interrupts disabled. If the mutex is not recursive the
 * mutex is locked only one level deep regardless of the depth value.
 * \param mutex mutex to be locked
 * \param d The instance of FastInterruptDisableLock used to disable interrupts
 * \param depth recursive depth at which the mutex will be locked. Zero
 * means the mutex is locked one level deep (as if lock() was called once),
 * one means two levels deep, etc. 
 */
static inline void IRQdoMutexLockToDepth(pthread_mutex_t *mutex,
        FastInterruptDisableLock& d, unsigned int depth)
{
    void *p=reinterpret_cast<void*>(Thread::IRQgetCurrentThread());
    if(mutex->owner==nullptr)
    {
        mutex->owner=p;
        if(mutex->recursive>=0) mutex->recursive=depth;
        return;
    }

    //This check is very important. Without this attempting to lock the same
    //mutex twice won't cause a deadlock because the wait is enclosed in a
    //while(owner!=p) which is immeditely false.
    if(mutex->owner==p)
    {
        if(mutex->recursive>=0)
        {
            mutex->recursive=depth;
            return;
        } else errorHandler(MUTEX_DEADLOCK); //Bad, deadlock
    }

    WaitingList waiting; //Element of a linked list on stack
    waiting.thread=p;
    waiting.next=nullptr; //Putting this thread last on the list (lifo policy)
    if(mutex->first==nullptr)
    {
        mutex->first=&waiting;
        mutex->last=&waiting;
    } else {
        mutex->last->next=&waiting;
        mutex->last=&waiting;
    }

    //The while is necessary to protect against spurious wakeups
    while(mutex->owner!=p) Thread::IRQenableIrqAndWait(d);
    if(mutex->recursive>=0) mutex->recursive=depth;
}

/**
 * \internal
 * Implementation code to unlock a mutex.
 * Must be called with interrupts disabled
 * \param mutex mutex to unlock
 * \return true if a higher priority thread was woken,
 * only if EDF scheduler is selected, otherwise it always returns false
 */
static inline bool IRQdoMutexUnlock(pthread_mutex_t *mutex)
{
//    Safety check removed for speed reasons
//    if(mutex->owner!=reinterpret_cast<void*>(Thread::IRQgetCurrentThread()))
//        return false;
    if(mutex->recursive>0)
    {
        mutex->recursive--;
        return false;
    }
    if(mutex->first!=nullptr)
    {
        Thread *t=reinterpret_cast<Thread*>(mutex->first->thread);
        t->IRQwakeup();
        mutex->owner=mutex->first->thread;
        mutex->first=mutex->first->next;

        #ifndef SCHED_TYPE_EDF
        if(Thread::IRQgetCurrentThread()->IRQgetPriority() < t->IRQgetPriority())
            return true;
        #endif //SCHED_TYPE_EDF
        return false;
    }
    mutex->owner=nullptr;
    return false;
}

/**
 * \internal
 * Implementation code to unlock all depth levels of a mutex.
 * Must be called with interrupts disabled
 * \param mutex mutex to unlock
 * \return the mutex recursive depth (how many times it was locked by the
 * owner). Zero means the mutex is locked one level deep (lock() was called
 * once), one means two levels deep, etc. 
 */
static inline unsigned int IRQdoMutexUnlockAllDepthLevels(pthread_mutex_t *mutex)
{
//    Safety check removed for speed reasons
//    if(mutex->owner!=reinterpret_cast<void*>(Thread::IRQgetCurrentThread()))
//        return false;
    if(mutex->first!=nullptr)
    {
        Thread *t=reinterpret_cast<Thread*>(mutex->first->thread);
        t->IRQwakeup();
        mutex->owner=mutex->first->thread;
        mutex->first=mutex->first->next;

        if(mutex->recursive<0) return 0;
        unsigned int result=mutex->recursive;
        mutex->recursive=0;
        return result;
    }
    
    mutex->owner=nullptr;
    
    if(mutex->recursive<0) return 0;
    unsigned int result=mutex->recursive;
    mutex->recursive=0;
    return result;
}

} //namespace miosix
