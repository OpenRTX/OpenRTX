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
#include <limits>

#ifdef SCHED_TYPE_EDF

namespace miosix {

class Thread; //Forward declaration

/**
 * This class models the concept of priority for the EDF scheduler.
 * Therefore, it represents a deadline, which is the absolute time withi which
 * the thread should have completed its computation.
 */
class EDFSchedulerPriority
{
public:
    /**
     * Constructor. Not explicit for backward compatibility.
     * \param deadline the thread deadline.
     */
    EDFSchedulerPriority(long long deadline) : deadline(deadline) {}

    /**
     * Default constructor.
     */
    EDFSchedulerPriority() : deadline(MAIN_PRIORITY) {}

    /**
     * \return the priority value
     */
    long long get() const { return deadline; }

    /**
     * \return true if this objects represents a valid deadline.
     */
    bool validate() const
    {
        // Deadlines must be positive, ant this is easy to understand.
        // The reason why numeric_limits<long long>::max()-1 is not allowed, is
        // because it is reserved for the idle thread.
        // Note that numeric_limits<long long>::max() is instead allowed, and
        // is used for thread that have no deadline assigned. In this way their
        // deadline comes after the deadline of the idle thread, and they never
        // run.
        return this->deadline>=0 &&
               this->deadline!=std::numeric_limits<long long>::max()-1;
    }
    
    /**
     * This function acts like a less-than operator but should be only used in
     * synchronization module for priority inheritance. The concept of priority
     * for preemption is not exactly the same for priority inheritance, hence,
     * this operation is a branch out of preemption priority for inheritance
     * purpose.
     */
    inline bool mutexLessOp(EDFSchedulerPriority b)
    {
        return deadline > b.deadline;
    }

private:
    long long deadline;///< The deadline time
};

inline bool operator<(EDFSchedulerPriority a, EDFSchedulerPriority b)
{
    //Not that the comparison is reversed on purpose. In fact, the thread which
    //has the lower deadline has higher priority
    return a.get() > b.get();
}

inline bool operator<=(EDFSchedulerPriority a, EDFSchedulerPriority b)
{
    //Not that the comparison is reversed on purpose. In fact, the thread which
    //has the lower deadline has higher priority
    return a.get() >= b.get();
}

inline bool operator>(EDFSchedulerPriority a, EDFSchedulerPriority b)
{
    //Not that the comparison is reversed on purpose. In fact, the thread which
    //has the lower deadline has higher priority
    return a.get() < b.get();
}

inline bool operator>=(EDFSchedulerPriority a, EDFSchedulerPriority b)
{
    //Not that the comparison is reversed on purpose. In fact, the thread which
    //has the lower deadline has higher priority
    return a.get() <= b.get();
}

inline bool operator==(EDFSchedulerPriority a, EDFSchedulerPriority b)
{
    return a.get() == b.get();
}

inline bool operator!=(EDFSchedulerPriority a, EDFSchedulerPriority b)
{
    return a.get() != b.get();
}

/**
 * \internal
 * An instance of this class is embedded in every Thread class. It contains all
 * the per-thread data required by the scheduler.
 */
class EDFSchedulerData
{
public:
    EDFSchedulerPriority deadline; ///<\internal thread deadline
    Thread *next=nullptr; ///<\internal list of threads, ordered by deadline
};

} //namespace miosix

#endif //SCHED_TYPE_EDF
