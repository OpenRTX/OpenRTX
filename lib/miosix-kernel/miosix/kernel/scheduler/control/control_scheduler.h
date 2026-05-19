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

#ifndef CONTROL_SCHEDULER_H
#define	CONTROL_SCHEDULER_H

#include "config/miosix_settings.h"
#include "control_scheduler_types.h"
#include "parameters.h"
#include "kernel/kernel.h"
#include <algorithm>

#ifdef SCHED_TYPE_CONTROL_BASED

namespace miosix {

/**
 * \internal
 * Control based scheduler.
 */
class ControlScheduler
{
public:
    /**
     * \internal
     * Add a new thread to the scheduler.
     * This is called when a thread is created.
     * \param thread a pointer to a valid thread instance.
     * The behaviour is undefined if a thread is added multiple timed to the
     * scheduler, or if thread is NULL.
     * \param priority the priority of the new thread.
     * Priority must be a positive value.
     * Note that the meaning of priority is scheduler specific.
     */
    static bool PKaddThread(Thread *thread, ControlSchedulerPriority priority);

    /**
     * \internal
     * \return true if thread exists, false if does not exist or has been
     * deleted. A joinable thread is considered existing until it has been
     * joined, even if it returns from its entry point (unless it is detached
     * and terminates).
     *
     * Can be called both with the kernel paused and with interrupts disabled.
     */
    static bool PKexists(Thread *thread);

    /**
     * \internal
     * Called when there is at least one dead thread to be removed from the
     * scheduler
     */
    static void PKremoveDeadThreads();

    /**
     * \internal
     * Set the priority of a thread.
     * Note that the meaning of priority is scheduler specific.
     * \param thread thread whose priority needs to be changed.
     * \param newPriority new thread priority.
     * Priority must be a positive value.
     */
    static void PKsetPriority(Thread *thread,
            ControlSchedulerPriority newPriority);

    /**
     * \internal
     * Get the priority of a thread.
     * Note that the meaning of priority is scheduler specific.
     * \param thread thread whose priority needs to be queried.
     * \return the priority of thread.
     */
    static ControlSchedulerPriority getPriority(Thread *thread)
    {
        return thread->schedData.priority;
    }

    /**
     * \internal
     * Same as getPriority, but meant to be called with interrupts disabled.
     * \param thread thread whose priority needs to be queried.
     * \return the priority of thread.
     */
    static ControlSchedulerPriority IRQgetPriority(Thread *thread)
    {
        return thread->schedData.priority;
    }

    /**
     * \internal
     * This is called before the kernel is started to by the kernel. The given
     * thread is the idle thread, to be run all the times where no other thread
     * can run.
     */
    static void IRQsetIdleThread(Thread *idleThread);

    /**
     * \internal
     * \return the idle thread.
     */
    static Thread *IRQgetIdleThread();

    /**
     * \internal
     * This member function is called by the kernel every time a thread changes
     * its running status. For example when a thread become sleeping, waiting,
     * deleted or if it exits the sleeping or waiting status
     */
    static void IRQwaitStatusHook()
    {
        #ifdef ENABLE_FEEDFORWARD
        IRQrecalculateAlfa();
        #endif //ENABLE_FEEDFORWARD
    }

    /**
     * \internal
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
    static void IRQfindNextThread();

private:

    /**
     * \internal
     * When priorities are modified, this function recalculates alfa for each
     * thread. Must be called with kernel paused
     */
    static void IRQrecalculateAlfa();

    /**
     * Called by IRQfindNextThread(), this function is where the control based
     * scheduling algorithm is run. It is called once per round.
     */
    static void IRQrunRegulator(bool allReadyThreadsSaturated)
    {
        using namespace std;
        #ifdef SCHED_CONTROL_FIXED_POINT
        //The fixed point scheduler may overflow if Tr is higher than this
        Tr=min(Tr,524287);
        #endif //FIXED_POINT_MATH
        #ifdef ENABLE_REGULATOR_REINIT
        if(reinitRegulator==false)
        {
        #endif //ENABLE_REGULATOR_REINIT
            int eTr=SP_Tr-Tr;
            #ifndef SCHED_CONTROL_FIXED_POINT
            int bc=bco+static_cast<int>(krr*eTr-krr*zrr*eTro);
            #else //FIXED_POINT_MATH
            //Tr is clamped to 524287, so eTr uses at most 19bits. Considering
            //the 31bits of a signed int, we have 12bits free.
            const int fixedKrr=static_cast<int>(krr*2048);
            const int fixedKrrZrr=static_cast<int>(krr*zrr*1024);
            int bc=bco+(fixedKrr*eTr)/2048-(fixedKrrZrr*eTro)/1024;
            #endif //FIXED_POINT_MATH
            if(allReadyThreadsSaturated)
            {
                //If all inner regulators reached upper saturation,
                //allow only a decrease in the burst correction.
                if(bc<bco) bco=bc;
            } else bco=bc;

            bco=min<int>(max(bco,-Tr),bMax*threadListSize);
            #ifndef SCHED_CONTROL_FIXED_POINT
            float nextRoundTime=static_cast<float>(Tr+bco);
            #else //FIXED_POINT_MATH
            unsigned int nextRoundTime=Tr+bco; //Bounded to 20bits
            #endif //FIXED_POINT_MATH
            eTro=eTr;
            Tr=0;//Reset round time
            for(Thread *it=threadList;it!=0;it=it->schedData.next)
            {
                //Recalculate per thread set point
                #ifndef SCHED_CONTROL_FIXED_POINT
                it->schedData.SP_Tp=static_cast<int>(
                        it->schedData.alfa*nextRoundTime);
                #else //FIXED_POINT_MATH
                //nextRoundTime is bounded to 20bits, alfa to 12bits,
                //so the multiplication fits in 32bits
                it->schedData.SP_Tp=(it->schedData.alfa*nextRoundTime)/4096;
                #endif //FIXED_POINT_MATH

                //Run each thread internal regulator
                int eTp=it->schedData.SP_Tp - it->schedData.Tp;
                //note: since b and bo contain the real value multiplied by
                //multFactor, this equals b=bo+eTp/multFactor.
                int b=it->schedData.bo + eTp;
                //saturation
                it->schedData.bo=min(max(b,bMin*multFactor),bMax*multFactor);
            }
        #ifdef ENABLE_REGULATOR_REINIT
        } else {
            reinitRegulator=false;
            Tr=0;//Reset round time
            //Reset state of the external regulator
            eTro=0;
            bco=0;

            for(Thread *it=threadList;it!=0;it=it->schedData.next)
            {
                //Recalculate per thread set point
                #ifndef SCHED_CONTROL_FIXED_POINT
                it->schedData.SP_Tp=static_cast<int>(it->schedData.alfa*SP_Tr);
                #else //FIXED_POINT_MATH
                //SP_Tr is bounded to 20bits, alfa to 12bits,
                //so the multiplication fits in 32bits
                it->schedData.SP_Tp=(it->schedData.alfa*SP_Tr)/4096;
                #endif //FIXED_POINT_MATH

                int b=it->schedData.SP_Tp*multFactor;
                it->schedData.bo=min(max(b,bMin*multFactor),bMax*multFactor);
            }
        }
        #endif //ENABLE_REGULATOR_REINIT
    }

    ///\internal Threads (except idle thread) are stored here
    static Thread *threadList;
    static unsigned int threadListSize;

    ///\internal current thread in the round
    static Thread *curInRound;

    ///\internal idle thread
    static Thread *idle;

    ///\internal Set point of round time
    ///Current policy = bNominal * actual # of threads
    static int SP_Tr;

    ///\internal Round time.
    static int Tr;

    ///\internal old burst correction
    static int bco;

    ///\internal old round tome error
    static int eTro;

    ///\internal set to true by IRQrecalculateAlfa() to signal that
    ///due to a change in alfa the regulator needs to be reinitialized
    static bool reinitRegulator;
};

} //namespace miosix

#endif //SCHED_TYPE_CONTROL_BASED

#endif //CONTROL_SCHEDULER_H
