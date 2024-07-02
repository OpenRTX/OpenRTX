/***************************************************************************
 *   Copyright (C) 2011 by Terraneo Federico                               *
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
#include "parameters.h"
#include "kernel/intrusive.h"

#ifdef SCHED_TYPE_CONTROL_BASED

namespace miosix {

class Thread; //Forward declaration

/**
 * Real-time priority definitions 
 * Applied to realtime field of ControlSchedulerPriority class.
 * 
 * When a thread wakes up (due to a prior Thread::sleep or waiting
 * for a hardware irq or etc.), the actual time the thread will have
 * its burst remainder will depend on the real-time priority set for it.
 */
enum class ControlRealtimePriority : unsigned char
{
    /**
     * REALTIME_PRIORITY_IMMEDIATE: The processor control is transfered to the thread
     * right in the time it wakes up.
     */
    REALTIME_PRIORITY_IMMEDIATE=0,
    /**
     * REALTIME_PRIORITY_NEXT_BURST: The processor control is transfered to the thread
     * right after the current running thread has consumed its burst time.
     */
    REALTIME_PRIORITY_NEXT_BURST=1,
    /**
     * REALTIME_PRIORITY_END_OF_ROUND: The processor control is transfered to the 
     * thread in the end of the round and the thread is delayed until all remaining 
     * active threads are run.
     */ 
    REALTIME_PRIORITY_END_OF_ROUND=2
};

/**
 * This class models the concept of priority for the control based scheduler.
 * In this scheduler the priority is simply a short int with values ranging
 * from 0 to PRIORITY_MAX-1, higher values mean higher priority, and the special
 * value -1 reserved for the idle thread.
 * Higher values of priority mean that the scheduler assigns a larger fraction
 * of the round time to the thread.
 */
class ControlSchedulerPriority
{
public:
    /**
     * Constructor. Not explicit for backward compatibility.
     * \param priority the desired priority value.
     */
    ControlSchedulerPriority(short int priority) : priority(priority),
            realtime(ControlRealtimePriority::REALTIME_PRIORITY_END_OF_ROUND) {}

    /**
     * Set both priority (proportional to burst length) and realtime priority
     * (whether immediate preemption is required)
     */
    ControlSchedulerPriority(short int priority, ControlRealtimePriority realtime)
            : priority(priority), realtime(realtime) {}

    /**
     * Default constructor.
     */
    ControlSchedulerPriority() : priority(MAIN_PRIORITY),
            realtime(ControlRealtimePriority::REALTIME_PRIORITY_END_OF_ROUND) {}

    /**
     * \return the priority value
     */
    short int get() const { return priority; }
    
    /**
     * \return the realtime priority
     */
    ControlRealtimePriority getRealtime() const { return realtime; }

    /**
     * \return true if this objects represents a valid priority.
     * Note that the value -1 is considered not valid, because it is reserved
     * for the idle thread.
     */
    bool validate() const
    {
        return priority>=0 && priority<PRIORITY_MAX &&
               static_cast<unsigned char>(realtime)<3;
    }
    
    /**
     * This function acts like a less-than operator but should be only used in
     * synchronization module for priority inheritance. The concept of priority
     * for preemption is not exactly the same for priority inheritance, hence,
     * this operation is a branch out of preemption priority for inheritance
     * purpose.
     */
    inline bool mutexLessOp(ControlSchedulerPriority b)
    {
        return priority < b.priority;
    }

private:
    short int priority;///< The priority value
    ControlRealtimePriority realtime;///< The realtime priority value
};

inline bool operator<(ControlSchedulerPriority a, ControlSchedulerPriority b)
{
    #ifdef SCHED_CONTROL_MULTIBURST
    //rule 1) Idle thread should be always preempted by any thread!
    //rule 2) Only REALTIME_PRIORITY_IMMEDIATE threads can preempt other threads
    //      a\b  idle  normal   immedaite
    // idle       f       T       T
    // normal     f       f       T
    // immediate  f       f       f
    return (a.get()==-1 && b.get()!=-1) ||
        (a.getRealtime()!=ControlRealtimePriority::REALTIME_PRIORITY_IMMEDIATE &&
         b.getRealtime()==ControlRealtimePriority::REALTIME_PRIORITY_IMMEDIATE);
    #else
    return a.get()<b.get();
    #endif
}

inline bool operator<=(ControlSchedulerPriority a, ControlSchedulerPriority b)
{
    #ifdef SCHED_CONTROL_MULTIBURST
    //      a\b  idle  normal   immedaite
    // idle       T       T       T
    // normal     f       T       T
    // immediate  f       f       T
    return (a.get()==-1 || b.get()!=-1) &&
        (a.getRealtime()!=ControlRealtimePriority::REALTIME_PRIORITY_IMMEDIATE ||
         b.getRealtime()==ControlRealtimePriority::REALTIME_PRIORITY_IMMEDIATE);
    #else
    return a.get()<=b.get();
    #endif
}

inline bool operator>(ControlSchedulerPriority a, ControlSchedulerPriority b)
{
    #ifdef SCHED_CONTROL_MULTIBURST
    //      a\b  idle  normal   immedaite
    // idle       f       f       f
    // normal     T       f       f
    // immediate  T       T       f
    return (a.get()!=-1 && b.get()==-1) ||
        (a.getRealtime()==ControlRealtimePriority::REALTIME_PRIORITY_IMMEDIATE &&
         b.getRealtime()!=ControlRealtimePriority::REALTIME_PRIORITY_IMMEDIATE);
    #else
    return a.get()>b.get();
    #endif
}

inline bool operator>=(ControlSchedulerPriority a, ControlSchedulerPriority b)
{
    #ifdef SCHED_CONTROL_MULTIBURST
    //      a\b  idle  normal   immedaite
    // idle       T       f       f
    // normal     T       T       f
    // immediate  T       T       T
    return (a.get()!=-1 || b.get()==-1) &&
        (a.getRealtime()==ControlRealtimePriority::REALTIME_PRIORITY_IMMEDIATE ||
         b.getRealtime()!=ControlRealtimePriority::REALTIME_PRIORITY_IMMEDIATE);
    #else
    return a.get()>=b.get();
    #endif
}

inline bool operator==(ControlSchedulerPriority a, ControlSchedulerPriority b)
{
    #ifdef SCHED_CONTROL_MULTIBURST
    return (a.getRealtime()==b.getRealtime()) && (a.get()==b.get());
    #else
    return a.get()==b.get();
    #endif
}

inline bool operator!=(ControlSchedulerPriority a, ControlSchedulerPriority b)
{
    #ifdef SCHED_CONTROL_MULTIBURST
    return (a.getRealtime()!=b.getRealtime()) || (a.get()!=b.get());
    #else
    return a.get()!=b.get();
    #endif
}

struct ThreadsListItem : public IntrusiveListItem
{
    Thread *t=nullptr;
};

/**
 * \internal
 * An instance of this class is embedded in every Thread class. It contains all
 * the per-thread data required by the scheduler.
 */
class ControlSchedulerData
{
public:
    ControlSchedulerData() : bo(bNominal*multFactor), alfa(0),
            SP_Tp(0), Tp(bNominal), next(nullptr) {}

    //Thread priority. Higher priority means longer burst
    ControlSchedulerPriority priority;
    int bo;//Old burst time, is kept here multiplied by multFactor
    #ifndef SCHED_CONTROL_FIXED_POINT
    float alfa; //Sum of all alfa=1
    #else //FIXED_POINT_MATH
    //Sum of all alfa is 4096 except for some rounding error
    unsigned short alfa;
    #endif //FIXED_POINT_MATH
    int SP_Tp;//Processing time set point
    int Tp;//Real processing time
    Thread *next;//Next thread in list
    ThreadsListItem atlEntry; //Entry in activeThreads list
    bool lastReadyStatus;
};

} //namespace miosix

#endif //SCHED_TYPE_CONTROL_BASED
