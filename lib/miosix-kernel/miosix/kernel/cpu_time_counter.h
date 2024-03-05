/***************************************************************************
 *   Copyright (C) 2023 by Daniele Cattaneo                                *
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

#include "kernel.h"
#include "cpu_time_counter_types.h"

#ifdef WITH_CPU_TIME_COUNTER

namespace miosix {

/**
 * \addtogroup Kernel
 * \{
 */

/**
 * CPUTimeCounter provides a low-level method to retrieve information about how
 * much CPU time was used up to now by each thread in the system.
 * It is intended for debugging and evaluation purposes and is enabled only if
 * the symbol `WITH_CPU_TIME_COUNTER` has been defined in
 * config/miosix_settings.h.
 * 
 * The implementation of this class collects this data by intercepting context
 * switch events. Due to the measurement method, some caveats apply to the data
 * returned:
 *  - There is no distinction between time spent in thread code or in the
 *    kernel.
 *  - Time spent in an interrupt is accounted towards the thread that has been
 *    interrupted.
 * 
 * Retrieving the time accounting data for all threads is performed through the
 * iterator returned by PKbegin(). To prevent the thread list from changing
 * because of a context switch, keep the kernel paused while you traverse the
 * iterator.
 * 
 * To simplify post-processing, the list of thread data information accessible
 * through the iterator always satisfies the following properties:
 *  - There is at least one item in the list.
 *  - The first item corresponds to the idle thread.
 *  - The threads are listed in creation order.
 *  - The relative order in which the items are iterated is deterministic and
 *    does not change even after a context switch.
 * 
 * These properties allow to compute the difference between two thread data
 * lists collected at different times in O(max(n,m)) complexity.
 * 
 * \note This is a very low-level interface. For actual use, a more practical
 * alternative is miosix::CPUProfiler, which provides a top-like display of the
 * amount of CPU used by each thread in a given time interval.
 */
class CPUTimeCounter
{
public:
    /**
     * Struct used to return the time counter data for a specific thread.
     */
    struct Data
    {
        /// The thread the data belongs to
        Thread *thread;
        /// Cumulative amount of CPU time scheduled to the thread in ns
        long long usedCpuTime = 0; 
    };

    /**
     * CPUTimeCounter thread data iterator type
     */
    class iterator
    {
    public:
        inline iterator operator++()
        {
            cur = cur->timeCounterData.next;
            return *this;
        }
        inline iterator operator++(int)
        {
            iterator result = *this;
            cur = cur->timeCounterData.next;
            return result;
        }
        inline Data operator*()
        {
            Data res;
            res.thread = cur;
            res.usedCpuTime = cur->timeCounterData.usedCpuTime;
            return res;
        }
        inline bool operator==(const iterator& rhs) { return cur==rhs.cur; }
        inline bool operator!=(const iterator& rhs) { return cur!=rhs.cur; }
    private:
        friend class CPUTimeCounter;
        Thread *cur;
        iterator(Thread *cur) : cur(cur) {}
    };

    /**
     * \returns the number of threads currently alive in the system.
     * \warning This method is only provided for the purpose of reserving enough
     * memory for collecting the time data for all threads. The value it
     * returns may change at any time.
     */
    static inline unsigned int getThreadCount()
    {
        return nThreads;
    }

    /**
     * \returns the begin iterator for the thread data.
     */
    static iterator PKbegin()
    {
        return iterator(head);
    }

    /**
     * \returns the end iterator for the thread data.
     */
    static iterator PKend()
    {
        return iterator(nullptr);
    }

    /**
     * \returns the amount of CPU run-time consumed up to now by the currently
     * active thread.
     */
    static long long getActiveThreadTime();

private:
    // The following methods are called from basic_scheduler to notify
    // CPUTimeCounter of various events.
    template<typename> friend class basic_scheduler;

    // CPUTimeCounter cannot be constructed
    CPUTimeCounter() = delete;

    /**
     * \internal
     * Add the idle thread to the list of threads tracked by CPUTimeCounter.
     * \param thread The idle thread.
     */
    static inline void IRQaddIdleThread(Thread *thread)
    {
        thread->timeCounterData.next = head;
        head = thread;
        if(!tail) tail = thread;
        nThreads++;
    }

    /**
     * \internal
     * Add an item to the list of threads tracked by CPUTimeCounter.
     * \param thread The thread to be added.
     */
    static inline void PKaddThread(Thread *thread)
    {
        tail->timeCounterData.next = thread;
        tail = thread;
        if(!head) head = thread;
        nThreads++;
    }

    /**
     * \internal
     * Update the list of threads tracked by CPUTimeCounter to remove dead
     * threads.
     */
    static void PKremoveDeadThreads();
    
    static Thread *head; ///< Head of the thread list
    static Thread *tail; ///< Tail of the thread list
    static volatile unsigned int nThreads; ///< Number of threads in the list
};

/**
 * Function to be called in the context switch code to profile threads
 * \param prev time count struct of previously running thread
 * \param prev time count struct of thread to be scheduled next
 * \param t (approximate) current time, a time point taken somewhere during
 * the context switch code
 */
static inline void IRQprofileContextSwitch(CPUTimeCounterPrivateThreadData& prev,
    CPUTimeCounterPrivateThreadData& next, long long t)
{
    prev.usedCpuTime += t - prev.lastActivation;
    next.lastActivation = t;
}

/**
 * \}
 */

}

#endif // WITH_CPU_TIME_COUNTER
