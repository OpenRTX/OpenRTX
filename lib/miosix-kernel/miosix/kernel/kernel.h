/***************************************************************************
 *   Copyright (C) 2008-2023 by Terraneo Federico                          *
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
#include "interfaces/portability.h"
#include "kernel/scheduler/sched_types.h"
#include "stdlib_integration/libstdcpp_integration.h"
#include "intrusive.h"
#include "cpu_time_counter_types.h"

/**
 * \namespace miosix
 * All user available kernel functions, classes are inside this namespace.
 */
namespace miosix {
    
/**
 * \addtogroup Kernel
 * \{
 */

/**
 * Disable interrupts, if interrupts were enable prior to calling this function.
 * 
 * Please note that starting from Miosix 1.51 disableInterrupts() and
 * enableInterrupts() can be nested. You can therefore call disableInterrupts()
 * multiple times as long as each call is matched by a call to
 * enableInterrupts().<br>
 * This replaced disable_and_save_interrupts() and restore_interrupts()
 *
 * disableInterrupts() cannot be called within an interrupt routine, but can be
 * called before the kernel is started (and does nothing in this case)
 */
void disableInterrupts();

/**
 * Enable interrupts.<br>
 * Please note that starting from Miosix 1.51 disableInterrupts() and
 * enableInterrupts() can be nested. You can therefore call disableInterrupts()
 * multiple times as long as each call is matched by a call to
 * enableInterrupts().<br>
 * This replaced disable_and_save_interrupts() and restore_interrupts()
 *
 * enableInterrupts() cannot be called within an interrupt routine, but can be
 * called before the kernel is started (and does nothing in this case)
 */
void enableInterrupts();

/**
 * Fast version of disableInterrupts().<br>
 * Despite faster, it has a couple of preconditions:
 * - calls to fastDisableInterrupts() can't be nested
 * - it can't be used in code that is called before the kernel is started
 */
inline void fastDisableInterrupts()
{
    miosix_private::doDisableInterrupts();
}

/**
 * Fast version of enableInterrupts().<br>
 * Despite faster, it has a couple of preconditions:
 * - calls to fastDisableInterrupts() can't be nested
 * - it can't be used in code that is called before the kernel is started,
 * because it will (incorreclty) lead to interrupts being enabled before the
 * kernel is started
 */
inline void fastEnableInterrupts()
{
    miosix_private::doEnableInterrupts();
}

/**
 * This class is a RAII lock for disabling interrupts. This call avoids
 * the error of not reenabling interrupts since it is done automatically.
 */
class InterruptDisableLock
{
public:
    /**
     * Constructor, disables interrupts.
     */
    InterruptDisableLock()
    {
        disableInterrupts();
    }

    /**
     * Destructor, reenables interrupts
     */
    ~InterruptDisableLock()
    {
        enableInterrupts();
    }

private:
    //Unwanted methods
    InterruptDisableLock(const InterruptDisableLock& l);
    InterruptDisableLock& operator= (const InterruptDisableLock& l);
};

/**
 * This class allows to temporarily re enable interrpts in a scope where
 * they are disabled with an InterruptDisableLock.<br>
 * Example:
 * \code
 *
 * //Interrupts enabled
 * {
 *     InterruptDisableLock dLock;
 *
 *     //Now interrupts disabled
 *
 *     {
 *         InterruptEnableLock eLock(dLock);
 *
 *         //Now interrupts back enabled
 *     }
 *
 *     //Now interrupts again disabled
 * }
 * //Finally interrupts enabled
 * \endcode
 */
class InterruptEnableLock
{
public:
    /**
     * Constructor, enables back interrupts.
     * \param l the InteruptDisableLock that disabled interrupts. Note that
     * this parameter is not used internally. It is only required to prevent
     * erroneous use of this class by making an instance of it without an
     * active InterruptEnabeLock
     */
    InterruptEnableLock(InterruptDisableLock& l)
    {
        (void)l;
        enableInterrupts();
    }

    /**
     * Destructor.
     * Disable back interrupts.
     */
    ~InterruptEnableLock()
    {
        disableInterrupts();
    }

private:
    //Unwanted methods
    InterruptEnableLock(const InterruptEnableLock& l);
    InterruptEnableLock& operator= (const InterruptEnableLock& l);
};

/**
 * This class is a RAII lock for disabling interrupts. This call avoids
 * the error of not reenabling interrupts since it is done automatically.
 * As opposed to InterruptDisableLock, this version doesn't support nesting
 */
class FastInterruptDisableLock
{
public:
    /**
     * Constructor, disables interrupts.
     */
    FastInterruptDisableLock()
    {
        fastDisableInterrupts();
    }

    /**
     * Destructor, reenables interrupts
     */
    ~FastInterruptDisableLock()
    {
        fastEnableInterrupts();
    }

private:
    //Unwanted methods
    FastInterruptDisableLock(const FastInterruptDisableLock& l);
    FastInterruptDisableLock& operator= (const FastInterruptDisableLock& l);
};

/**
 * This class allows to temporarily re enable interrpts in a scope where
 * they are disabled with an FastInterruptDisableLock.
 */
class FastInterruptEnableLock
{
public:
    /**
     * Constructor, enables back interrupts.
     * \param l the InteruptDisableLock that disabled interrupts. Note that
     * this parameter is not used internally. It is only required to prevent
     * erroneous use of this class by making an instance of it without an
     * active InterruptEnabeLock
     */
    FastInterruptEnableLock(FastInterruptDisableLock& l)
    {
        (void)l;
        fastEnableInterrupts();
    }

    /**
     * Destructor.
     * Disable back interrupts.
     */
    ~FastInterruptEnableLock()
    {
        fastDisableInterrupts();
    }

private:
    //Unwanted methods
    FastInterruptEnableLock(const FastInterruptEnableLock& l);
    FastInterruptEnableLock& operator= (const FastInterruptEnableLock& l);
};

/**
 * Pause the kernel.<br>Interrupts will continue to occur, but no preemption is
 * possible. Call to this function are cumulative: if you call pauseKernel()
 * two times, you need to call restartKernel() two times.<br>Pausing the kernel
 * must be avoided if possible because it is easy to cause deadlock. Calling
 * file related functions (fopen, Directory::open() ...), serial port related
 * functions (printf ...) or kernel functions that cannot be called when the
 * kernel is paused will cause deadlock. Therefore, if possible, it is better to
 * use a Mutex instead of pausing the kernel<br>This function is safe to be
 * called even before the kernel is started. In this case it has no effect.
 */
void pauseKernel();

/**
 * Restart the kernel.<br>This function will yield immediately if a tick has
 * been missed. Since calls to pauseKernel() are cumulative, if you call
 * pauseKernel() two times, you need to call restartKernel() two times.<br>
 * This function is safe to be called even before the kernel is started. In this
 * case it has no effect.
 */
void restartKernel();

/**
 * \return true if interrupts are enabled
 */
bool areInterruptsEnabled();

/**
 * This class is a RAII lock for pausing the kernel. This call avoids
 * the error of not restarting the kernel since it is done automatically.
 */
class PauseKernelLock
{
public:
    /**
     * Constructor, pauses the kernel.
     */
    PauseKernelLock()
    {
        pauseKernel();
    }

    /**
     * Destructor, restarts the kernel
     */
    ~PauseKernelLock()
    {
        restartKernel();
    }

private:
    //Unwanted methods
    PauseKernelLock(const PauseKernelLock& l);
    PauseKernelLock& operator= (const PauseKernelLock& l);
};

/**
 * This class allows to temporarily restart kernel in a scope where it is
 * paused with an InterruptDisableLock.<br>
 * Example:
 * \code
 *
 * //Kernel started
 * {
 *     PauseKernelLock dLock;
 *
 *     //Now kernel paused
 *
 *     {
 *         RestartKernelLock eLock(dLock);
 *
 *         //Now kernel back started
 *     }
 *
 *     //Now kernel again paused
 * }
 * //Finally kernel started
 * \endcode
 */
class RestartKernelLock
{
public:
    /**
     * Constructor, restarts kernel.
     * \param l the PauseKernelLock that disabled interrupts. Note that
     * this parameter is not used internally. It is only required to prevent
     * erroneous use of this class by making an instance of it without an
     * active PauseKernelLock
     */
    RestartKernelLock(PauseKernelLock& l)
    {
        (void)l;
        restartKernel();
    }

    /**
     * Destructor.
     * Disable back interrupts.
     */
    ~RestartKernelLock()
    {
        pauseKernel();
    }

private:
    //Unwanted methods
    RestartKernelLock(const RestartKernelLock& l);
    RestartKernelLock& operator= (const RestartKernelLock& l);
};

/**
 * Prevent the microcontroller from entering a deep sleep state. Most commonly
 * used by device drivers requiring clocks or power rails that would be disabled
 * when entering deep sleep to perform blocking operations while informing the
 * scheduler that deep sleep is currently not possible.
 * Can be nested multiple times and called by different device drivers
 * simultaneously. If N calls to deepSleepLock() are made, then N calls to
 * deepSleepUnlock() need to be made before deep sleep is enabled back.
 */
void deepSleepLock();

/**
 * Used to signal the scheduler that a critical section where deep sleep should
 * not be entered has completed. If N calls to deepSleepLock() are made, then N
 * calls to deepSleepUnlock() need to be made before deep sleep is enabled back.
 */
void deepSleepUnlock();

/**
 * This class is a RAII lock for temporarily prevent entering deep sleep.
 * This call avoids the error of not reenabling deep sleep capability since it
 * is done automatically.
 */
class DeepSleepLock
{
public:       
    DeepSleepLock() { deepSleepLock(); }

    ~DeepSleepLock() { deepSleepUnlock(); }

private: 
    DeepSleepLock(const DeepSleepLock&);
    DeepSleepLock& operator= (const DeepSleepLock&);
};

/**
 * \internal
 * Start the kernel.<br> There is no way to stop the kernel once it is
 * started, except a (software or hardware) system reset.<br>
 * Calls errorHandler(OUT_OF_MEMORY) if there is no heap to create the idle
 * thread. If the function succeds in starting the kernel, it never returns;
 * otherwise it will call errorHandler(OUT_OF_MEMORY) and then return
 * immediately. startKernel() must not be called when the kernel is already
 * started.
 */
void startKernel();

/**
 * Return true if kernel is running, false if it is not started, or paused.<br>
 * Warning: disabling/enabling interrupts does not affect the result returned by
 * this function.
 * \return true if kernel is running (started && not paused)
 */
bool isKernelRunning();

/**
 * Returns OS time, which is a monotonic clock started when the OS booted.<br>
 * Warning! This function replaces the getTick() in previous versions of the
 * kernel, but unlike getTick(), getTime() cannot be called with interrupts
 * disabled. For that, you need to call IRQgeTime().
 * \return current time in nanoseconds
 */
long long getTime() noexcept;

/**
 * Returns OS time, which is a monotonic clock started when the OS booted.<br>
 * Must be called with interrupts disabled, or within an interrupt.
 * \return current time in nanoseconds
 */
long long IRQgetTime() noexcept;

/**
 * Possible return values of timedWait
 */
enum class TimedWaitResult
{
    NoTimeout,
    Timeout
};

//Forwrd declaration
class SleepData;
class MemoryProfiling;
class Mutex;
class ConditionVariable;
#ifdef WITH_PROCESSES
class ProcessBase;
#endif //WITH_PROCESSES

/**
 * This class represents a thread. It has methods for creating, deleting and
 * handling threads.<br>It has private constructor and destructor, since memory
 * for a thread is handled by the kernel.<br>To create a thread use the static
 * producer method create().<br>
 * Methods that have an effect on the current thread, that is, the thread that
 * is calling the method are static.<br>
 * Calls to non static methods must be done with care, because a thread can
 * terminate at any time. For example, if you call wakeup() on a terminated
 * thread, the behavior is undefined.
 */
class Thread
{
public:

    /**
     * Thread options, can be passed to Thread::create to set additional options
     * of the thread.
     * More options can be specified simultaneously by ORing them together.
     * The DEFAULT option indicates the default thread creation.
     */
    enum Options
    {
        DEFAULT=0,    ///< Default thread options
        JOINABLE=1<<0 ///< Thread is joinable instead of detached
    };

    /**
     * Producer method, creates a new thread.
     * \param startfunc the entry point function for the thread
     * \param stacksize size of thread stack, its minimum is the constant
     * STACK_MIN.
     * The size of the stack must be divisible by 4, otherwise it will be
     * rounded to a number divisible by 4.
     * \param priority the thread's priority, between 0 (lower) and
     * PRIORITY_MAX-1 (higher)
     * \param argv a void* pointer that is passed as pararmeter to the entry
     * point function
     * \param options thread options, such ad Thread::JOINABLE
     * \return a reference to the thread created, that can be used, for example,
     * to delete it, or nullptr in case of errors.
     *
     * Can be called when the kernel is paused.
     */
    static Thread *create(void *(*startfunc)(void *), unsigned int stacksize,
                            Priority priority=Priority(), void *argv=nullptr,
                            unsigned short options=DEFAULT);

    /**
     * Same as create(void (*startfunc)(void *), unsigned int stacksize,
     * Priority priority=1, void *argv=nullptr)
     * but in this case the entry point of the thread returns a void*
     * \param startfunc the entry point function for the thread
     * \param stacksize size of thread stack, its minimum is the constant
     * STACK_MIN.
     * The size of the stack must be divisible by 4, otherwise it will be
     * rounded to a number divisible by 4.
     * \param priority the thread's priority, between 0 (lower) and
     * PRIORITY_MAX-1 (higher)
     * \param argv a void* pointer that is passed as pararmeter to the entry
     * point function
     * \param options thread options, such ad Thread::JOINABLE
     * \return a reference to the thread created, that can be used, for example,
     * to delete it, or nullptr in case of errors.
     */
    static Thread *create(void (*startfunc)(void *), unsigned int stacksize,
                            Priority priority=Priority(), void *argv=nullptr,
                            unsigned short options=DEFAULT);

    /**
     * When called, suggests the kernel to pause the current thread, and run
     * another one.
     * <br>CANNOT be called when the kernel is paused.
     */
    static void yield();

    /**
     * Put the thread to sleep for a number of milliseconds.
     * <br>The actual precision depends on the underlying hardware timer.
     * \param ms the number of milliseconds. If it is ==0 this method will
     * return immediately
     *
     * CANNOT be called when the kernel is paused.
     */
    static void sleep(unsigned int ms);

    /**
     * Put the thread to sleep for a number of nanoseconds.
     * <br>The actual precision depends on the underlying hardware timer.
     * \param ns the number of nanoseconds. If it is <=0 this method will
     * return immediately
     *
     * CANNOT be called when the kernel is paused.
     */
    static void nanoSleep(long long ns);
    
    /**
     * Put the thread to sleep until the specified absolute time is reached.
     * If the time is in the past, returns immediately.
     * To make a periodic thread, this is the recomended way
     * \code
     * void periodicThread()
     * {
     *     const long long period=90000000; //Run every 90 milliseconds
     *     long long time=getTime();
     *     for(;;)
     *     {
     *         //Do work
     *         time+=period;
     *         Thread::nanoSleepUntil(time);
     *     }
     * }
     * \endcode
     * \param absoluteTime when to wake up, in nanoseconds
     *
     * CANNOT be called when the kernel is paused.
     */
    static void nanoSleepUntil(long long absoluteTimeNs);

    /**
     * This method stops the thread until wakeup() is called.
     * Ths method is useful to implement any kind of blocking primitive,
     * including device drivers.
     *
     * CANNOT be called when the kernel is paused.
     */
    static void wait();

    /**
     * This method stops the thread until wakeup() is called.
     * Ths method is useful to implement any kind of blocking primitive,
     * including device drivers.
     *
     * Note: this method is meant to put the current thread in wait status in a
     * piece of code where interrupts are disbled; it returns immediately, so
     * the user is responsible for re-enabling interrupts and calling yield to
     * effectively put the thread in wait status.
     *
     * \code
     * disableInterrupts();
     * ...
     * Thread::IRQwait(); //Return immediately
     * enableInterrupts();
     * Thread::yield(); //After this, thread is in wait status
     * \endcode
     *
     * Consider using IRQenableIrqAndWait() instead.
     */
    static void IRQwait();

    /**
     * This method stops the thread until wakeup() is called.
     * Ths method is useful to implement any kind of blocking primitive,
     * including device drivers.
     *
     * NOTE: this method is meant to put the current thread in wait status in a
     * piece of code where the kernel is paused (preemption disabled).
     * Preemption will be enabled during the waiting period, and disabled back
     * before this method returns.
     *
     * \param dLock the PauseKernelLock object that was used to disable
     * preemption in the current context.
     */
    static void PKrestartKernelAndWait(PauseKernelLock& dLock);

    /**
     * This method stops the thread until wakeup() is called.
     * Ths method is useful to implement any kind of blocking primitive,
     * including device drivers.
     *
     * NOTE: this method is meant to put the current thread in wait status in a
     * piece of code where interrupts are disbled, interrupts will be enabled
     * during the waiting period, and disabled back before this method returns.
     *
     * \param dLock the InterruptDisableLock object that was used to disable
     * interrupts in the current context.
     */
    static void IRQenableIrqAndWait(InterruptDisableLock& dLock)
    {
        (void)dLock; //Common implementation doesn't need it
        return IRQenableIrqAndWaitImpl();
    }

    /**
     * This method stops the thread until wakeup() is called.
     * Ths method is useful to implement any kind of blocking primitive,
     * including device drivers.
     *
     * NOTE: this method is meant to put the current thread in wait status in a
     * piece of code where interrupts are disbled, interrupts will be enabled
     * during the waiting period, and disabled back before this method returns.
     *
     * \param dLock the FastInterruptDisableLock object that was used to disable
     * interrupts in the current context.
     */
    static void IRQenableIrqAndWait(FastInterruptDisableLock& dLock)
    {
        (void)dLock; //Common implementation doesn't need it
        return IRQenableIrqAndWaitImpl();
    }

    /**
     * This method stops the thread until wakeup() is called or the specified
     * absolute time in nanoseconds is reached.
     * Ths method is thus a combined IRQwait() and absoluteSleep(), and is
     * useful to implement any kind of blocking primitive with timeout,
     * including device drivers.
     *
     * \param absoluteTimeoutNs absolute time after which the wait times out
     * \return TimedWaitResult::Timeout if the wait timed out
     */
    static TimedWaitResult timedWait(long long absoluteTimeNs)
    {
        FastInterruptDisableLock dLock;
        return IRQenableIrqAndTimedWaitImpl(absoluteTimeNs);
    }

    /**
     * This method stops the thread until wakeup() is called or the specified
     * absolute time in nanoseconds is reached.
     * Ths method is thus a combined IRQwait() and absoluteSleep(), and is
     * useful to implement any kind of blocking primitive with timeout,
     * including device drivers.
     *
     * NOTE: this method is meant to put the current thread in wait status in a
     * piece of code where the kernel is paused (preemption disabled).
     * Preemption will be enabled during the waiting period, and disabled back
     * before this method returns.
     *
     * \param dLock the PauseKernelLock object that was used to disable
     * preemption in the current context.
     * \param absoluteTimeoutNs absolute time after which the wait times out
     * \return TimedWaitResult::Timeout if the wait timed out
     */
    static TimedWaitResult PKrestartKernelAndTimedWait(PauseKernelLock& dLock,
            long long absoluteTimeNs);

    /**
     * This method stops the thread until wakeup() is called or the specified
     * absolute time in nanoseconds is reached.
     * Ths method is thus a combined IRQwait() and absoluteSleep(), and is
     * useful to implement any kind of blocking primitive with timeout,
     * including device drivers.
     *
     * NOTE: this method is meant to put the current thread in wait status in a
     * piece of code where interrupts are disbled, interrupts will be enabled
     * during the waiting period, and disabled back before this method returns.
     *
     * \param dLock the InterruptDisableLock object that was used to disable
     * interrupts in the current context.
     * \param absoluteTimeoutNs absolute time after which the wait times out
     * \return TimedWaitResult::Timeout if the wait timed out
     */
    static TimedWaitResult IRQenableIrqAndTimedWait(InterruptDisableLock& dLock,
            long long absoluteTimeNs)
    {
        (void)dLock; //Common implementation doesn't need it
        return IRQenableIrqAndTimedWaitImpl(absoluteTimeNs);
    }

    /**
     * This method stops the thread until wakeup() is called or the specified
     * absolute time in nanoseconds is reached.
     * Ths method is thus a combined IRQwait() and absoluteSleep(), and is
     * useful to implement any kind of blocking primitive with timeout,
     * including device drivers.
     *
     * NOTE: this method is meant to put the current thread in wait status in a
     * piece of code where interrupts are disbled, interrupts will be enabled
     * during the waiting period, and disabled back before this method returns.
     *
     * \param dLock the FastInterruptDisableLock object that was used to disable
     * interrupts in the current context.
     * \param absoluteTimeoutNs absolute time after which the wait times out
     * \return TimedWaitResult::Timeout if the wait timed out
     */
    static TimedWaitResult IRQenableIrqAndTimedWait(FastInterruptDisableLock& dLock,
            long long absoluteTimeNs)
    {
        (void)dLock; //Common implementation doesn't need it
        return IRQenableIrqAndTimedWaitImpl(absoluteTimeNs);
    }

    /**
     * Wakeup a thread.
     * <br>CANNOT be called when the kernel is paused.
     */
    void wakeup();

    /**
     * Wakeup a thread.
     * <br>Can only be called when the kernel is paused.
     */
    void PKwakeup();

    /**
     * Wakeup a thread.
     * <br>Can only be called inside an IRQ or when interrupts are disabled.
     */
    void IRQwakeup();
    
    /**
     * \return a pointer to the current thread.
     *
     * Returns a valid pointer also if called before the kernel is started.
     */
    static Thread *getCurrentThread()
    {
        //Safe to call without disabling IRQ, see implementation
        return IRQgetCurrentThread();
    }

    /**
     * \return a pointer to the current thread.
     *
     * Returns a valid pointer also if called before the kernel is started.
     */
    static Thread *PKgetCurrentThread()
    {
        //Safe to call without disabling IRQ, see implementation
        return IRQgetCurrentThread();
    }

    /**
     * \return a pointer to the current thread.
     *
     * Returns a valid pointer also if called before the kernel is started.
     */
    static Thread *IRQgetCurrentThread();

    /**
     * Check if a thread exists
     * \param p thread to check
     * \return true if thread exists, false if does not exist or has been
     * deleted. A joinable thread is considered existing until it has been
     * joined, even if it returns from its entry point (unless it is detached
     * and terminates).
     *
     * Can be called when the kernel is paused.
     */
    static bool exists(Thread *p);

    /**
     * Returns the priority of a thread.<br>
     * To get the priority of the current thread use:
     * \code Thread::getCurrentThread()->getPriority(); \endcode
     * If the thread is currently locking one or more mutexes, this member
     * function returns the current priority, which can be higher than the
     * original priority due to priority inheritance.
     * \return current priority of the thread
     */
    Priority getPriority();

    /**
     * Same as getPriority(), but meant to be used when the kernel is paused.
     */
    Priority PKgetPriority()
    {
        return getPriority(); //Safe to call directly, see implementation
    }

    /**
     * Same as getPriority(), but meant to be used inside an IRQ, or when
     * interrupts are disabled.
     */
    Priority IRQgetPriority()
    {
        return getPriority(); //Safe to call directly, see implementation
    }

    /**
     * Set the priority of this thread.<br>
     * This member function changed from previous Miosix versions since it is
     * now static. This implies a thread can no longer set the priority of
     * another thread.
     * \param pr desired priority. Must be 0<=pr<PRIORITY_MAX
     *
     * Can be called when the kernel is paused.
     */
    static void setPriority(Priority pr);

    /**
     * Suggests a thread to terminate itself. Note that this method only makes
     * testTerminate() return true on the specified thread. If the thread does
     * not call testTerminate(), or if it calls it but does not delete itself
     * by returning from entry point function, it will NEVER
     * terminate. The user is responsible for implementing correctly this
     * functionality.<br>Thread termination is implemented like this to give
     * time to a thread to deallocate resources, close files... before
     * terminating.<br>The first call to terminate on a thread will make it
     * return prematurely form wait(), sleep() and timedWait() call, but only
     * once.<br>Can be called when the kernel is paused.
     */
    void terminate();

    /**
     * This method needs to be called periodically inside the thread's main
     * loop.
     * \return true if somebody outside the thread called terminate() on this
     * thread.
     *
     * If it returns true the thread must free all resources and terminate by
     * returning from its main function.
     * <br>Can be called when the kernel is paused.
     */
    static bool testTerminate();

    /**
     * Detach the thread if it was joinable, otherwise do nothing.<br>
     * If called on a deleted joinable thread on which join was not yet called,
     * it allows the thread's memory to be deallocated.<br>
     * If called on a thread that is not yet deleted, the call detaches the
     * thread without deleting it.
     * If called on an already detached thread, it has undefined behaviour.
     */
    void detach();

    /**
     * \return true if the thread is detached
     */
    bool isDetached() const;

    /**
     * Wait until a joinable thread is terminated.<br>
     * If the thread already terminated, this function returns immediately.<br>
     * Calling join() on the same thread multiple times, from the same or
     * multiple threads is not recomended, but in the current implementation
     * the first call will wait for join, and the other will return false.<br>
     * Trying to join the thread join is called in returns false, but must be
     * avoided.<br>
     * Calling join on a detached thread might cause undefined behaviour.
     * \param result If the entry point function of the thread to join returns
     * void *, the return value of the entry point is stored here, otherwise
     * the content of this variable is undefined. If nullptr is passed as result
     * the return value will not be stored.
     * \return true on success, false on failure
     */
    bool join(void** result=nullptr);

    /**
     * \internal
     * This method is only meant to implement functions to check the available
     * stack in a thread. Returned pointer is constant because modifying the
     * stack through it must be avoided.
     * \return pointer to bottom of stack of current thread.
     */
    static const unsigned int *getStackBottom();

    /**
     * \internal
     * \return the size of the stack of the current thread.
     */
    static int getStackSize();

    /**
     * \internal
     * Used before every context switch to check if the stack of the thread
     * being preempted has overflowed
     */
    static void IRQstackOverflowCheck();
    
    #ifdef WITH_PROCESSES

    /**
     * \return the process associated with the thread 
     */
    ProcessBase *getProcess() { return proc; }
    
    /**
     * \internal
     * Can only be called inside an IRQ, its use is to switch a thread between
     * userspace/kernelspace and back to perform context switches
     */
    static void IRQhandleSvc(unsigned int svcNumber);
    
    /**
     * \internal
     * Can only be called inside an IRQ, its use is to report a fault so that
     * in case the fault has occurred within a process while it was executing
     * in userspace, the process can be terminated.
     * \param fault data about the occurred fault
     * \return true if the fault was caused by a process, false otherwise.
     */
    static bool IRQreportFault(const miosix_private::FaultData& fault);
    
    #endif //WITH_PROCESSES

    //Unwanted methods
    Thread(const Thread& p) = delete;
    Thread& operator = (const Thread& p) = delete;

private:
    /**
     * Curren thread status
     */
    class ThreadFlags
    {
    public:
        /**
         * Constructor, sets flags to default.
         */
        ThreadFlags(Thread *t) : t(t), flags(0) {}

        /**
         * Set the wait flag of the thread.
         * Can only be called with interrupts disabled or within an interrupt.
         * \param waiting if true the flag will be set, otherwise cleared
         */
        void IRQsetWait(bool waiting);

        /**
         * Set the sleep flag of the thread.
         * Can only be called with interrupts disabled or within an interrupt.
         */
        void IRQsetSleep();

        /**
         * Used by IRQwakeThreads to clear both the sleep and wait flags,
         * waking threads doing absoluteSleep() as well as timedWait()
         */
        void IRQclearSleepAndWait();

        /**
         * Set the wait_join flag of the thread.
         * Can only be called with interrupts disabled or within an interrupt.
         * \param waiting if true the flag will be set, otherwise cleared
         */
        void IRQsetJoinWait(bool waiting);

        /**
         * Set the deleted flag of the thread. This flag can't be cleared.
         * Can only be called with interrupts disabled or within an interrupt.
         */
        void IRQsetDeleted();

        /**
         * Set the sleep flag of the thread. This flag can't be cleared.
         * Can only be called with interrupts disabled or within an interrupt.
         */
        void IRQsetDeleting()
        {
            flags |= DELETING;
        }

        /**
         * Set the detached flag. This flag can't be cleared.
         * Can only be called with interrupts disabled or within an interrupt.
         */
        void IRQsetDetached()
        {
            flags |= DETACHED;
        }
        
        /**
         * Set the userspace flag of the thread.
         * Can only be called with interrupts disabled or within an interrupt.
         * \param sleeping if true the flag will be set, otherwise cleared
         */
        void IRQsetUserspace(bool userspace)
        {
            if(userspace) flags |= USERSPACE; else flags &= ~USERSPACE;
        }

        /**
         * \return true if the wait flag is set
         */
        bool isWaiting() const { return flags & WAIT; }

        /**
         * \return true if the sleep flag is set
         */
        bool isSleeping() const { return flags & SLEEP; }

        /**
         * \return true if the deleted and the detached flags are set
         */
        bool isDeleted() const { return (flags & 0x14)==0x14; }

        /**
         * \return true if the thread has been deleted, but its resources cannot
         * be reclaimed because it has not yet been joined
         */
        bool isDeletedJoin() const { return flags & DELETED; }

        /**
         * \return true if the deleting flag is set
         */
        bool isDeleting() const { return flags & DELETING; }

        /**
         * \return true if the thread is in the ready status
         */
        bool isReady() const { return (flags & 0x27)==0; }

        /**
         * \return true if the thread is detached
         */
        bool isDetached() const { return flags & DETACHED; }

        /**
         * \return true if the thread is waiting a join
         */
        bool isWaitingJoin() const { return flags & WAIT_JOIN; }
        
        /**
         * \return true if the thread is running unprivileged inside a process.
         */
        bool isInUserspace() const { return flags & USERSPACE; }

        //Unwanted methods
        ThreadFlags(const ThreadFlags& p) = delete;
        ThreadFlags& operator = (const ThreadFlags& p) = delete;

    private:
        ///\internal Thread is in the wait status. A call to wakeup will change
        ///this
        static const unsigned int WAIT=1<<0;

        ///\internal Thread is sleeping.
        static const unsigned int SLEEP=1<<1;

        ///\internal Thread is deleted. It will continue to exist until the
        ///idle thread deallocates its resources
        static const unsigned int DELETED=1<<2;

        ///\internal Somebody outside the thread asked this thread to delete
        ///itself.<br>This will make Thread::testTerminate() return true.
        static const unsigned int DELETING=1<<3;

        ///\internal Thread is detached
        static const unsigned int DETACHED=1<<4;

        ///\internal Thread is waiting for a join
        static const unsigned int WAIT_JOIN=1<<5;
        
        ///\internal Thread is running in userspace
        static const unsigned int USERSPACE=1<<6;

        Thread* t; ///<\internal pointer to the thread to which the flags belong
        unsigned char flags;///<\internal flags are stored here
    };
    
    #ifdef WITH_PROCESSES

    /**
     * \internal
     * Causes a thread belonging to a process to switch to userspace, and
     * execute userspace code. This function returns when the process performs
     * a syscall or faults.
     * \return the syscall parameters used to serve the system call.
     */
    static miosix_private::SyscallParameters switchToUserspace();

    /**
     * Create a thread to be used inside a process. The thread is created in
     * WAIT status, a wakeup() on it is required to actually start it.
     * \param startfunc entry point
     * \param argv parameter to be passed to the entry point
     * \param options thread options
     * \param proc process to which this thread belongs
     */
    static Thread *createUserspace(void *(*startfunc)(void *),
        void *argv, unsigned short options, Process *proc);
    
    /**
     * Setup the userspace context of the thread, so that it can be later
     * switched to userspace. Must be called only once for each thread instance
     * \param entry userspace entry point
     * \param gotBase base address of the GOT, also corresponding to the start
     * of the RAM image of the process
     * \param ramImageSize size of the process ram image
     */
    static void setupUserspaceContext(unsigned int entry, unsigned int *gotBase,
        unsigned int ramImageSize);
    
    #endif //WITH_PROCESSES

    /**
     * Constructor, initializes thread data.
     * \param watermark pointer to watermark area
     * \param stacksize thread's stack size
     * \param defaultReent true if the global reentrancy structure is to be used
     */
    Thread(unsigned int *watermark, unsigned int stacksize, bool defaultReent);

    /**
     * Destructor
     */
    ~Thread();
    
    /**
     * Helper function to initialize a Thread
     * \param startfunc entry point function
     * \param stacksize stack size for the thread
     * \param argv argument passed to the thread entry point
     * \param options thread options
     * \param defaultReent true if the default C reentrancy data should be used
     * \return a pointer to a thread, or nullptr in case there are not enough
     * resources to create one.
     */
    static Thread *doCreate(void *(*startfunc)(void *), unsigned int stacksize,
                            void *argv, unsigned short options, bool defaultReent);

    /**
     * Thread launcher, all threads start from this member function, which calls
     * the user specified entry point. When the entry point function returns,
     * it marks the thread as deleted so that the idle thread can dellocate it.
     * If exception handling is enebled, this member function also catches any
     * exception that propagates through the entry point.
     * \param threadfunc pointer to the entry point function
     * \param argv argument passed to the entry point
     */
    static void threadLauncher(void *(*threadfunc)(void*), void *argv);

    /**
     * Common implementation of all IRQenableIrqAndWait calls
     */
    static void IRQenableIrqAndWaitImpl();

    /**
     * Common implementation of all timedWait calls
     */
    static TimedWaitResult IRQenableIrqAndTimedWaitImpl(long long absoluteTimeNs);

    /**
     * Same as exists() but is meant to be called only inside an IRQ or when
     * interrupts are disabled.
     */
    static bool IRQexists(Thread *p);

    /**
     * Allocates the idle thread and makes cur point to it
     * Can only be called before the kernel is started, is called exactly once
     * so that getCurrentThread() always returns a pointer to a valid thread or
     * by startKernel to create the idle thread, whichever comes first.
     * \return the newly allocated idle thread
     */
    static Thread *allocateIdleThread();
    
    /**
     * \return the C reentrancy structure of the currently running thread
     */
    static struct _reent *getCReent();

    //Thread data
    SchedulerData schedData; ///< Scheduler data, only used by class Scheduler
    ThreadFlags flags;///< thread status
    ///Saved priority. Its value is relevant only if mutexLockedCount>0; it
    ///stores the value of priority that this thread will have when it unlocks
    ///all mutexes. This is because when a thread locks a mutex its priority
    ///can change due to priority inheritance.
    Priority savedPriority;
    ///List of mutextes locked by this thread
    Mutex *mutexLocked;
    ///If the thread is waiting on a Mutex, mutexWaiting points to that Mutex
    Mutex *mutexWaiting;
    unsigned int *watermark;///< pointer to watermark area
    unsigned int ctxsave[CTXSAVE_SIZE];///< Holds cpu registers during ctxswitch
    unsigned int stacksize;///< Contains stack size
    ///This union is used to join threads. When the thread to join has not yet
    ///terminated and no other thread called join it contains (Thread *)nullptr,
    ///when a thread calls join on this thread it contains the thread waiting
    ///for the join, and when the thread terminated it contains (void *)result
    union
    {
        Thread *waitingForJoin;///<Thread waiting to join this
        void *result;          ///<Result returned by entry point
    } joinData;
    /// Per-thread instance of data to make the C and C++ libraries thread safe.
    struct _reent *cReentrancyData;
    CppReentrancyData cppReentrancyData;
    #ifdef WITH_PROCESSES
    ///Process to which this thread belongs. Null if it is a kernel thread.
    ProcessBase *proc;
    ///Pointer to the set of saved registers for when the thread is running in
    ///user mode. For kernel threads (i.e, threads where proc==kernel) this
    ///pointer is null
    unsigned int *userCtxsave;
    #endif //WITH_PROCESSES
    #ifdef WITH_CPU_TIME_COUNTER
    CPUTimeCounterPrivateThreadData timeCounterData;
    #endif //WITH_CPU_TIME_COUNTER
    
    //friend functions
    //Needs access to flags
    friend bool IRQwakeThreads(long long);
    //Needs to create the idle thread
    friend void startKernel();
    //Needs threadLauncher
    friend void miosix_private::initCtxsave(unsigned int *, void *(*)(void *),
            unsigned int *, void *);
    //Needs access to priority, savedPriority, mutexLocked and flags.
    friend class Mutex;
    //Needs access to flags, schedData
    friend class PriorityScheduler;
    //Needs access to flags, schedData
    friend class ControlScheduler;
    //Needs access to flags, schedData
    friend class EDFScheduler;
    //Needs access to cppReent
    friend class CppReentrancyAccessor;
    #ifdef WITH_PROCESSES
    //Needs PKcreateUserspace(), setupUserspaceContext(), switchToUserspace()
    friend class Process;
    #endif //WITH_PROCESSES
    #ifdef WITH_CPU_TIME_COUNTER
    //Needs access to timeCounterData
    friend class CPUTimeCounter;
    #endif //WITH_CPU_TIME_COUNTER
};

/**
 * \internal
 * This class is used to make a list of sleeping threads.
 * It is used by the kernel, and should not be used by end users.
 */
class SleepData : public IntrusiveListItem
{
public:
    SleepData(Thread *thread, long long wakeupTime)
        : thread(thread), wakeupTime(wakeupTime) {}

    ///\internal Thread that is sleeping
    Thread *thread;
    
    ///\internal When this number becomes equal to the kernel tick,
    ///the thread will wake
    long long wakeupTime;
};

/**
 * \}
 */

} //namespace miosix
