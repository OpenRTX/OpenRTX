/***************************************************************************
 *   Copyright (C) 2010, 2011 by Terraneo Federico                         *
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

#include "config/miosix_settings.h"
#include "kernel/scheduler/priority/priority_scheduler.h"
#include "kernel/scheduler/control/control_scheduler.h"
#include "kernel/scheduler/edf/edf_scheduler.h"
#include "kernel/cpu_time_counter.h"

namespace miosix {

class Thread; //Forward declaration

/**
 * \internal
 * This class is the common interface between the kernel and the scheduling
 * algorithms.
 * Dispatching of the calls to the implementation is done using templates
 * instead of inheritance and virtual functions beacause the scheduler
 * implementation is chosen at compile time.
 */
template<typename T>
class basic_scheduler
{
public:
    /**
     * \internal
     * Add a new thread to the scheduler.
     * \param thread a pointer to a valid thread instance.
     * The behaviour is undefined if a thread is added multiple timed to the
     * scheduler, or if thread is NULL.
     * \param priority the priority of the new thread.
     * Priority must be a positive value.
     * Note that the meaning of priority is scheduler specific.
     * \return false if an error occurred and the thread could not be added to
     * the scheduler
     *
     * Note: this member function is called also before the kernel is started
     * to add the main and idle thread.
     */
    static bool PKaddThread(Thread *thread, Priority priority)
    {
        bool res=T::PKaddThread(thread,priority);
        #ifdef WITH_CPU_TIME_COUNTER
        if(res) CPUTimeCounter::PKaddThread(thread);
        #endif
        return res;
    }

    /**
     * \internal
     * \return true if thread exists, false if does not exist or has been
     * deleted. A joinable thread is considered existing until it has been
     * joined, even if it returns from its entry point (unless it is detached
     * and terminates).
     *
     * Can be called both with the kernel paused and with interrupts disabled.
     */
    static bool PKexists(Thread *thread)
    {
        return T::PKexists(thread);
    }

    /**
     * \internal
     * Called when there is at least one dead thread to be removed from the
     * scheduler
     */
    static void PKremoveDeadThreads()
    {
        #ifdef WITH_CPU_TIME_COUNTER
        CPUTimeCounter::PKremoveDeadThreads();
        #endif
        T::PKremoveDeadThreads();
    }

    /**
     * \internal
     * Set the priority of a thread.
     * Note that the meaning of priority is scheduler specific.
     * \param thread thread whose priority needs to be changed.
     * \param newPriority new thread priority.
     * Priority must be a positive value.
     */
    static void PKsetPriority(Thread *thread, Priority newPriority)
    {
        T::PKsetPriority(thread,newPriority);
    }

    /**
     * \internal
     * Get the priority of a thread. Must be callable also with kernel paused
     * or IRQ disabled.
     * Note that the meaning of priority is scheduler specific.
     * \param thread thread whose priority needs to be queried.
     * \return the priority of thread.
     */
    static Priority getPriority(Thread *thread)
    {
        return T::getPriority(thread);
    }

    /**
     * \internal
     * This is called before the kernel is started to by the kernel. The given
     * thread is the idle thread, to be run all the times where no other thread
     * can run.
     */
    static void IRQsetIdleThread(Thread *idleThread)
    {
        #ifdef WITH_CPU_TIME_COUNTER
        CPUTimeCounter::IRQaddIdleThread(idleThread);
        #endif
        return T::IRQsetIdleThread(idleThread);
    }

    /**
     * \internal
     * This member function is called by the kernel every time a thread changes
     * its running status. For example when a thread become sleeping, waiting,
     * deleted or if it exits the sleeping or waiting status
     */
    static void IRQwaitStatusHook(Thread *t)
    {
        T::IRQwaitStatusHook(t);
    }

    /**
     * This function is used to develop interrupt driven peripheral drivers.<br>
     * Can be used ONLY inside an IRQ (and not when interrupts are disabled) to
     * find next thread in READY status. If the kernel is paused, does nothing.
     * Can be used for example if an IRQ causes a higher priority thread to be
     * woken, to change context. Note that to use this function the IRQ must
     * use the macros to save/restore context defined in portability.h
     *
     * If the kernel is paused does nothing.
     * It's behaviour is to modify the global variable miosix::cur which always
     * points to the currently running thread.
     */
    static void IRQfindNextThread()
    {
        T::IRQfindNextThread();
    }
    
    /**
     * \internal
     * \return the next scheduled preemption set by the scheduler
     * In case no preemption is set returns numeric_limits<long long>::max()
     */
    static long long IRQgetNextPreemption()
    {
        return T::IRQgetNextPreemption();
    }
};

#ifdef SCHED_TYPE_PRIORITY
typedef basic_scheduler<PriorityScheduler> Scheduler;
#elif defined(SCHED_TYPE_CONTROL_BASED)
typedef basic_scheduler<ControlScheduler> Scheduler;
#elif defined(SCHED_TYPE_EDF)
typedef basic_scheduler<EDFScheduler> Scheduler;
#else
#error No scheduler selected in config/miosix_settings.h
#endif

} //namespace miosix
