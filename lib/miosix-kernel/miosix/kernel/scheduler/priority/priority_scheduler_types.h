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

#ifdef SCHED_TYPE_PRIORITY

namespace miosix {

class Thread; //Forward declaration

/**
 * This class models the concept of priority for the priority scheduler.
 * In this scheduler the priority is simply a short int with values ranging
 * from 0 to PRIORITY_MAX-1, higher values mean higher priority, and the special
 * value -1 reserved for the idle thread.
 */
class PrioritySchedulerPriority
{
public:
    /**
     * Constructor. Not explicit for backward compatibility.
     * \param priority the desired priority value.
     */
    PrioritySchedulerPriority(short int priority) : priority(priority) {}

    /**
     * Default constructor.
     */
    PrioritySchedulerPriority() : priority(MAIN_PRIORITY) {}

    /**
     * \return the priority value
     */
    short int get() const { return priority; }

    /**
     * \return true if this objects represents a valid priority.
     * Note that the value -1 is considered not valid, because it is reserved
     * for the idle thread.
     */
    bool validate() const
    {
        return this->priority>=0 && this->priority<PRIORITY_MAX;
    }
    
    /**
     * This function acts like a less-than operator but should be only used in
     * synchronization module for priority inheritance. The concept of priority
     * for preemption is not exactly the same for priority inheritance, hence,
     * this operation is a branch out of preemption priority for inheritance
     * purpose.
     */
    inline bool mutexLessOp(PrioritySchedulerPriority b)
    {
        return priority < b.priority;
    }

private:
    short int priority;///< The priority value
};

inline bool operator<(PrioritySchedulerPriority a, PrioritySchedulerPriority b)
{
    return a.get() < b.get();
}

inline bool operator<=(PrioritySchedulerPriority a, PrioritySchedulerPriority b)
{
    return a.get() <= b.get();
}

inline bool operator>(PrioritySchedulerPriority a, PrioritySchedulerPriority b)
{
    return a.get() > b.get();
}

inline bool operator>=(PrioritySchedulerPriority a, PrioritySchedulerPriority b)
{
    return a.get() >= b.get();
}

inline bool operator==(PrioritySchedulerPriority a, PrioritySchedulerPriority b)
{
    return a.get() == b.get();
}

inline bool operator!=(PrioritySchedulerPriority a, PrioritySchedulerPriority b)
{
    return a.get() != b.get();
}

/**
 * \internal
 * An instance of this class is embedded in every Thread class. It contains all
 * the per-thread data required by the scheduler.
 */
class PrioritySchedulerData
{
public:
    ///Thread priority. Used to speed up the implementation of getPriority.<br>
    ///Note that to change the priority of a thread it is not enough to change
    ///this.<br>It is also necessary to move the thread from the old prority
    ///list to the new priority list.
    PrioritySchedulerPriority priority;
    Thread *next;///<Pointer to next thread of the same priority. CIRCULAR list
};

} //namespace miosix

#endif //SCHED_TYPE_PRIORITY
