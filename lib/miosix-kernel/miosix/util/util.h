/***************************************************************************
 *   Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013 by Terraneo Federico *
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

/***********************************************************************
* util.h Part of the Miosix Embedded OS.
* A collection of "utilities".
************************************************************************/

#ifndef UTIL_H
#define UTIL_H

#include "kernel/cpu_time_counter.h"
#include <vector>

namespace miosix {

/**
 * \addtogroup Util
 * \{
 */

/**
 * This class allows to gather memory statistics useful when developing
 * embedded code.
 */
class MemoryProfiling
{
public:

    /**
     * Prints a summary of the information that can be gathered from this class.
     */
    static void print();

    /**
     * \return stack size of current thread.
     */
    static unsigned int getStackSize();

    /**
     * \return absolute free stack of current thread.<br>
     * Absolute free stack is the minimum free stack since the thread was
     * created.
     */
    static unsigned int getAbsoluteFreeStack();

    /**
     * \return current free stack  of current thread.<br>
     * Current free stack is the free stack at the moment when the this
     * function is called.
     */
    static unsigned int getCurrentFreeStack();

    /**
     * \return heap size which is defined in the linker script.<br>The heap is
     * shared among all threads, therefore this function returns the same value
     * regardless which thread is called in.
     */
    static unsigned int getHeapSize();

    /**
     * \return absolute (not current) free heap.<br>
     * Absolute free heap is the minimum free heap since the program started.
     * <br>The heap is shared among all threads, therefore this function returns
     * the same value regardless which thread is called in.
     */
    static unsigned int getAbsoluteFreeHeap();

    /**
     * \return current free heap.<br>
     * Current free heap is the free heap at the moment when the this
     * function is called.<br>
     * The heap is shared among all threads, therefore this function returns
     * the same value regardless which thread is called in.
     */
    static unsigned int getCurrentFreeHeap();

private:
    //All member functions static, disallow creating instances
    MemoryProfiling();
};

/**
 * Dump a memory area in this format
 * 0x00000000 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 | ................
 * \param start pointer to beginning of memory block to dump
 * \param len length of memory block to dump
 */
void memDump(const void *start, int len);

#ifdef WITH_CPU_TIME_COUNTER

/**
 * This class implements a top-like view of the CPU usage of all current
 * threads. The implementation is built upon CPUTimeCounter and therefore also
 * requires `WITH_CPU_TIME_COUNTER` to be defined in config/miosix_settings.h.
 * 
 * CPUProfiler can be integrated in an existing update loop by instantiating it
 * and then invoking the update() and print() methods at regular intervals:
 * 
 *      CPUProfiler p;
 *      long long t = p.update();
 *      while (...)
 *      {
 *          t += 1000000000LL;
 *          Thread::nanoSleepUntil(t);
 *          p.update();
 *          p.print();
 *          ...
 *      }
 * 
 * The profiler will automatically keep track of the actual update period.
 * 
 * Alternatively, you can use the static thread() method as the main function
 * of a thread which will implement the logic shown above to periodically
 * print profiling information on stdout:
 * 
 *      std::thread profThread(CPUProfiler::thread, 1000000000LL);
 * 
 * It is recommended to guard CPUTimeCounter with `#ifdef WITH_CPU_TIME_COUNTER`
 * to automatically remove all related code when CPUTimeCounter is not available
 * (i.e. in release builds).
 */
class CPUProfiler
{
public:
    /**
     * Construct a new profiler object.
     */
    CPUProfiler() {}

    /**
     * Update the profiler status with the latest information about all
     * threads in the system.
     * \returns the time (in nanoseconds) at which the data was collected.
     */
    long long update();

    /**
     * Prints the profiling information to stdout in a tabular top-like display.
     */
    void print();

    /**
     * Continuously collects and print CPU usage information for all threads.
     * Returns once Thread::testTerminate() returns `true'.
     * \param nsInterval The interval between subsequent CPU usage information
     * printouts.
     * \warning The current implementation can wait up to `nsInterval'
     * nanoseconds before the thread termination condition is noticed.
     */
    static void thread(long long nsInterval);

private:
    /**
     * \internal
     * Structure containing a snapshot of the CPU data information returned
     * by CPUTimeCounter for each thread.
     */
    struct Snapshot
    {
        /// The thread data objects, one per thread
        std::vector<CPUTimeCounter::Data> threadData;
        /// The time (in ns) at which the snapshot was collected
        long long time = 0;

        /**
         * Fills the snapshot with information collected from CPUTimeCounter.
         * Discards the previous content of the snapshot.
         */
        void collect();
    };

    /// Two thread data snapshots used for computing the amount of CPU time
    /// used by each thread between the two.
    Snapshot snapshots[2];
    /// Which of the two snapshots is the last one collected.
    unsigned int lastSnapshotIndex = 0;
};

#endif // WITH_CPU_TIME_COUNTER

/**
 * \}
 */

} //namespace miosix

#endif //UTIL_H
