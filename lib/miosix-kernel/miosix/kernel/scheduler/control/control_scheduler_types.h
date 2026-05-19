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

#include "config/miosix_settings.h"
#include "parameters.h"

#ifndef CONTROL_SCHEDULER_TYPES_H
#define	CONTROL_SCHEDULER_TYPES_H

#ifdef SCHED_TYPE_CONTROL_BASED

namespace miosix {

class Thread; //Forward declaration

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
    ControlSchedulerPriority(short int priority): priority(priority) {}

    /**
     * Default constructor.
     */
    ControlSchedulerPriority(): priority(MAIN_PRIORITY) {}

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

private:
    short int priority;///< The priority value
};

inline bool operator <(ControlSchedulerPriority a, ControlSchedulerPriority b)
{
    return a.get() < b.get();
}

inline bool operator <=(ControlSchedulerPriority a, ControlSchedulerPriority b)
{
    return a.get() <= b.get();
}

inline bool operator >(ControlSchedulerPriority a, ControlSchedulerPriority b)
{
    return a.get() > b.get();
}

inline bool operator >=(ControlSchedulerPriority a, ControlSchedulerPriority b)
{
    return a.get() >= b.get();
}

inline bool operator ==(ControlSchedulerPriority a, ControlSchedulerPriority b)
{
    return a.get() == b.get();
}

inline bool operator !=(ControlSchedulerPriority a, ControlSchedulerPriority b)
{
    return a.get() != b.get();
}

/**
 * \internal
 * An instance of this class is embedded in every Thread class. It contains all
 * the per-thread data required by the scheduler.
 */
class ControlSchedulerData
{
public:
    ControlSchedulerData(): priority(0), bo(bNominal*multFactor), alfa(0),
            SP_Tp(0), Tp(bNominal), next(0) {}

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
};

} //namespace miosix

#endif //SCHED_TYPE_CONTROL_BASED

#endif //CONTROL_SCHEDULER_TYPES_H
