/***************************************************************************
 *   Copyright (C) 2015-2021 by Terraneo Federico, Sasan Golchin           *
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

#include "kernel/timeconversion.h"
#include "kernel/scheduler/timer_interrupt.h"

/**
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file os_timer.h
 * This file contains the interface through which the OS accesses the underlying
 * hardware timer, that is used to:
 * - measure time durations
 * - set interrupts used both preemption and to handle sleeping threads wakeup
 *
 * Please note that all functions in this interface should provide the kernel
 * with time information in nanoseconds. In the platform-specific implementation
 * it is highly recommended to use the TimeConversion class to convert the
 * underlying hardware timer ticks to nanoseconds.
 * 
 * NOTE: when porting Miosix, architectures providing
 * - a timer counting up
 * - a match register capable of generating interrupts
 * - an overflow interrupt
 * can simply derive the TimerAdapter providing the required functions to
 * access the hardware timer, and the DEFAULT_OS_TIMER_INTERFACE_IMPLMENTATION
 * macro to implement the os_timer interface.
 */

namespace miosix {

// The os timer platform-specific implementation shall provide these two
// functions, although they are not declared here, but in kernel.h, as
// these are the only two function that are meant to be called also from
// application code. For comments about the intended behavior, see kernel.h
//long long getTime() noexcept;
//long long IRQgetTime() noexcept;
    
namespace internal {

/**
 * \internal
 * Initialize and start the os timer.
 * It is used by the kernel, and should not be used by end users.
 */
void IRQosTimerInit();

/**
 * \internal
 * Set the next interrupt.
 * It is used by the kernel, and should not be used by end users.
 * Can be called with interrupts disabled or within an interrupt.
 * The hardware timer handles only one outstading interrupt request at a
 * time, so a new call before the interrupt expires cancels the previous one.
 * \param ns the absolute time when the interrupt will be fired, in nanoseconds.
 * When the interrupt fires, it shall call the
 * \code
 * void IRQtimerInterrupt(long long currentTime);
 * \endcode
 * function defined in kernel/scheduler/timer_interrupt.h
 */
void IRQosTimerSetInterrupt(long long ns) noexcept;

/**
 * \internal
 * Set the current system time.
 * It is used by the kernel, and should not be used by end users.
 * Used to adjust the time for example if the system clock was stopped due to
 * entering deep sleep.
 * Can be called with interrupts disabled or within an interrupt.
 * \param ns value to set the hardware timer to. Note that the timer can
 * only be set to a higher value, never to a lower one, as the OS timer
 * needs to be monotonic.
 * If an interrupt has been set with IRQsetNextInterrupt, it needs to be
 * moved accordingly or fired immediately if the timer advance causes it
 * to be in the past.
 */ 
void IRQosTimerSetTime(long long ns) noexcept;

/**
 * \internal
 * It is used by the kernel, and should not be used by end users.
 * \return the timer frequency in Hz.
 * If a prescaler is used, it should be taken into account, the returned
 * value should be equal to the frequency at which the timer increments in
 * an observable way through IRQgetCurrentTime()
 */
unsigned int osTimerGetFrequency();

} //namespace internal

/**
 * Helper class providing a generic implementation capable of providing time
 * in nanoseconds starting from a hardware timer.
 * It works by first extending the timer counter to 64 bit in software through
 * an algorithm called the "pending bit trick" and then using TimeConversion to
 * turn the timer ticks to nanoseconds.
 * 
 * This class is not meant to be instantiated directly, but rather used as a
 * base class by means of the C++ curiously recurring template pattern.
 * In the derived class you need to implement function to perform
 * platform-specific functions on the hardware timer, as shown by this example
 * code.
 * \code
 * class MyHwTimer : public TimerAdapter<MyHwTimer, insert timer bits here>
 * {
 * public:
 *     static inline unsigned int IRQgetTimerCounter() {}
 *     static inline void IRQsetTimerCounter(unsigned int v) {}
 * 
 *     static inline unsigned int IRQgetTimerMatchReg() {}
 *     static inline void IRQsetTimerMatchReg(unsigned int v) {}
 * 
 *     static inline bool IRQgetOverflowFlag() {}
 *     static inline void IRQclearOverflowFlag() {}
 *     
 *     static inline bool IRQgetMatchFlag() {}
 *     static inline void IRQclearMatchFlag() {}
 *     
 *     static inline void IRQforcePendingIrq() {}
 * 
 *     static inline void IRQstopTimer() {}
 *     static inline void IRQstartTimer() {}
 * 
 *     static unsigned int IRQTimerFrequency() {}
 * 
 *     static void IRQinit() {}
 * };
 * \endcode
 * 
 * \tparam D the derived class (see curiously recurring template pattern)
 * \tparam bits the bits of the underlying hardware timer, up to 32 bit.
 * \tparam quirkAdvance some timers don't like being set very close to the
 * actual interrupt time. If this is the case set this parameter to the minimum
 * number of ticks in the future the timer must be set, otherwise keep at 0 
 */
template<typename D, unsigned bits, unsigned quirkAdvance=0>
class TimerAdapter
{
public:
    //Note that if you have a 64 bit timer you don't need this code at all
    static_assert(bits<=32, "Support for larger timers not implemented");
    static constexpr unsigned long long upperIncr=(1LL<<bits);
    static constexpr unsigned long long lowerMask=upperIncr-1;
    static constexpr unsigned long long upperMask=0xFFFFFFFFFFFFFFFFLL-lowerMask;
    
    long long upperTimeTick = 0; //Extended timer counter (upper bits)
    long long upperIrqTick = 0;  //Extended interrupt time point (upper bits)
    miosix::TimeConversion tc = TimeConversion(D::IRQTimerFrequency());
    bool lateIrq=false;
    
    /**
     * \return the current time in ticks
     */
    inline long long IRQgetTimeTick()
    {
        // THE PENDING BIT TRICK, version 2
        // This algorithm allows to extend in software an N bit timer to a 64bit
        // one. The basic idea is this: the lower bits of the 64bit timer are
        // kept by the counter register of the timer, while the upper bits are
        // kept in a software variable. When the hardware timer overflows, an
        // interrupt is used to update the upper bits.
        // Reading the timer may appear to be doable by just an OR operation
        // between the software variable and the hardware counter, but is
        // actually way trickier than it seems, because user code may:
        // 1 disable interrupts,
        // 2 spend a little time with interrupts disabled,
        // 3 call this function.
        // Now, if a timer overflow occurs while interrupts are disabled, the
        // upper bits have not yet been updated by the overflow interrupt, so
        // we would return the wrong time.
        // To fix this, we check the timer overflow pending bit, and if it is
        // set we return the time adjusted accordingly. This almost works, the
        // last issue to fix is that reading the timer counter and the pending
        // bit is not an atomic operation, and the counter may roll over exactly
        // at that point in time. In this case we must not increment the upper
        // bits at all. To solve this, we read the timer a second time to see if
        // it had rolled over.
        // This is the pending bit trick, that in a nutshell uses the overflow
        // pending flag as an extra timer bit, and accounts for the
        // impossibility to atomically read the timer counter and pending flag.
        // Note that this algorithm imposes a limit on the maximum time
        // interrupts can be disabeld, equals to one hardware timer period minus
        // the time between the two timer reads in this algorithm.
        unsigned int counter=D::IRQgetTimerCounter();
        if(D::IRQgetOverflowFlag() && D::IRQgetTimerCounter()>=counter)
            return (upperTimeTick | static_cast<long long>(counter)) + upperIncr;
        return upperTimeTick | static_cast<long long>(counter);
    }
    
    /**
     * \return the time when the next os interrupt is scheduled in ticks
     */
    inline long long IRQgetIrqTick()
    {
        return upperIrqTick | D::IRQgetTimerMatchReg();
    }
    
    /**
     * \return the current time in nanoseconds
     */
    inline long long IRQgetTimeNs()
    {
        return tc.tick2ns(IRQgetTimeTick());
    }
    
    /**
     * \return the time when the next os interrupt is scheduled in nanoseconds
     */
    inline long long IRQgetIrqNs()
    {
        return tc.tick2ns(IRQgetIrqTick());
    }
    
    /**
     * Set the current time
     * \param ns absolute time in nanoseconds, can only be greater than the
     * current time
     */
    void IRQsetTimeNs(long long ns)
    {
        //Normally we never stop the timer not to accumulate clock skew,
        //but here we're asked to introduce a clock jump anyway
        D::IRQstopTimer();
        long long oldTick = IRQgetTimeTick();
        long long tick = tc.ns2tick(ns);
        if(tick>oldTick)
        {
            upperTimeTick = tick & upperMask;
            D::IRQsetTimerCounter(static_cast<unsigned int>(tick & lowerMask));
            D::IRQclearOverflowFlag();
            //Adjust also when the next interrupt will be fired
            long long nextIrqTick = IRQgetIrqTick();
            if(nextIrqTick>oldTick)
            {
                //Avoid using IRQsetIrqTick(nextIrqTick) as in some weird timers
                //IRQgetTimeTick() does not work after setting the timer counter
                //and before starting the timer (ATsam4l is an example)
                auto tick2 = nextIrqTick + quirkAdvance;
                upperIrqTick = tick2 & upperMask;
                D::IRQsetTimerMatchReg(static_cast<unsigned int>(tick2 & lowerMask));
                if(tick >= nextIrqTick)
                {
                    D::IRQforcePendingIrq();
                    lateIrq=true;
                }
            }
        }
        D::IRQstartTimer();
    }
    
    /**
     * Schedule the next os interrupt
     * \param ns absolute time in ticks, must be > 0
     */
    inline void IRQsetIrqTick(long long tick)
    {
        auto tick2 = tick + quirkAdvance;
        upperIrqTick = tick2 & upperMask;
        D::IRQsetTimerMatchReg(static_cast<unsigned int>(tick2 & lowerMask));
        if(IRQgetTimeTick() >= tick)
        {
            D::IRQforcePendingIrq();
            lateIrq=true;
        }
    }
    
    /**
     * Schedule the next os interrupt
     * \param ns absolute time in nanoseconds, must be > 0
     */
    inline void IRQsetIrqNs(long long ns)
    {
        IRQsetIrqTick(tc.ns2tick(ns));
    }
    
    /**
     * Must be called by the timer interrupt routine when writing the driver
     * for a particular timer. It clears the pending flag and calls the os as
     * needed.
     */
    inline void IRQhandler()
    {
        if(D::IRQgetMatchFlag() || lateIrq)
        {
            D::IRQclearMatchFlag();
            long long tick=IRQgetTimeTick();
            if(tick >= IRQgetIrqTick() || lateIrq)
            {
                lateIrq=false;
                IRQtimerInterrupt(tc.tick2ns(tick));
            }
        }
        if(D::IRQgetOverflowFlag())
        {
            D::IRQclearOverflowFlag();
            upperTimeTick += upperIncr;
        }
    }
    
    /**
     * Initializes and starts the timer.
     */
    void IRQinit()
    {
        D::IRQinitTimer();
        D::IRQstartTimer();
    }
};

} //namespace miosix

/**
 * This macro is a shorthand for implementing the os timer interface in terms of
 * the TimerAdapter class. Just declare this macro <b>inside the miosix
 * namespace</b> passing it the TimerAdapter derived class instance.
 * 
 * \code
 * namespace miosix {
 * class MyHwTimer : public TimerAdapter<MyHwTimer, insert timer bits here>
 * {
 *     [...]
 * };
 * 
 * static MyHwTimer timer;
 * DEFAULT_OS_TIMER_INTERFACE_IMPLMENTATION(timer);
 * }
 * 
 * void timerInterruptRoutine()
 * {
 *     miosix::timer.IRQhandler();
 * }
 * \endcode
 */
#define DEFAULT_OS_TIMER_INTERFACE_IMPLMENTATION(timer) \
long long getTime() noexcept                       \
{                                                  \
    FastInterruptDisableLock dLock;                \
    return timer.IRQgetTimeNs();                   \
}                                                  \
                                                   \
long long IRQgetTime() noexcept                    \
{                                                  \
    return timer.IRQgetTimeNs();                   \
}                                                  \
                                                   \
namespace internal {                               \
                                                   \
void IRQosTimerInit()                              \
{                                                  \
    timer.IRQinit();                               \
}                                                  \
                                                   \
void IRQosTimerSetInterrupt(long long ns) noexcept \
{                                                  \
    timer.IRQsetIrqNs(ns);                         \
}                                                  \
                                                   \
void IRQosTimerSetTime(long long ns) noexcept      \
{                                                  \
    timer.IRQsetTimeNs(ns);                        \
}                                                  \
                                                   \
unsigned int osTimerGetFrequency()                 \
{                                                  \
    FastInterruptDisableLock dLock;                \
    return timer.IRQTimerFrequency();              \
}                                                  \
                                                   \
} //namespace internal

/**
 * \}
 */
