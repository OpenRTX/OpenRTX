/***************************************************************************
 *   Copyright (C) 2010-2021 by Terraneo Federico                          *
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

//For SCHED_TYPE_* config options
#include "config/miosix_settings.h"
//For MPUConfiguration
#include "core/memory_protection.h"
#include <cstddef>

/**
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file portability.h
 * This file is the interface from the Miosix kernel to the hardware.
 * It ccontains what is required to perform a context switch, disable
 * interrupts, set up the stack frame and registers of a newly created thread,
 * and contains iterrupt handlers for preemption and yield.
 *
 * Since some of the functions in this file must be inline for speed reasons,
 * and context switch code must be macros, at the end of this file the file
 * portability_impl.h is included.
 * This file should contain the implementation of those inline functions.
 */

#ifdef WITH_PROCESSES

namespace miosix {

class Process; //Forward decl

} //namespace miosix

#endif //WITH_PROCESSES

/**
 * \}
 */

/**
 * \namespace miosix_pivate
 * contains architecture-specific functions. These functions are separated from
 * the functions in kernel.h because:<br>
 * - to port the kernel to another processor you only need to rewrite these
 *   functions.
 * - these functions are only useful for writing hardare drivers, most user code
 *  does not need them.
 */
namespace miosix_private {

/**
 * \addtogroup Interfaces
 * \{
 */

   
  
/**
 * \internal
 * Used after an unrecoverable error condition to restart the system, even from
 * within an interrupt routine.
 */
void IRQsystemReboot();

/**
 * \internal
 * Cause a context switch.
 * It is used by the kernel, and should not be used by end users.
 */
inline void doYield();

/**
 * \internal
 * Initializes a ctxsave array when a thread is created.
 * It is used by the kernel, and should not be used by end users.
 * \param ctxsave a pointer to a field ctxsave inside a Thread class that need
 * to be filled
 * \param pc starting program counter of newly created thread, used to
 * initialize ctxsave
 * \param sp starting stack pointer of newly created thread, used to initialize
 * ctxsave
 * \param argv starting data passed to newly created thread, used to initialize
 * ctxsave
 */
void initCtxsave(unsigned int *ctxsave, void *(*pc)(void *), unsigned int *sp,
        void *argv);

#ifdef WITH_PROCESSES

/**
 * This class allows to access the parameters that a process passed to
 * the operating system as part of a system call
 */
class SyscallParameters
{
public:
    /**
     * Constructor, initialize the class starting from the thread's userspace
     * context
     */
    SyscallParameters(unsigned int *context);
    
    /**
     * \return the syscall id, used to identify individual system calls
     */
    int getSyscallId() const;
    
    /**
     * \return the first syscall parameter. The returned result is meaningful
     * only if the syscall (identified through its id) has one or more parameters
     */
    unsigned int getFirstParameter() const;
    
    /**
     * \return the second syscall parameter. The returned result is meaningful
     * only if the syscall (identified through its id) has two or more parameters
     */
    unsigned int getSecondParameter() const;
    
    /**
     * \return the third syscall parameter. The returned result is meaningful
     * only if the syscall (identified through its id) has three parameters
     */
    unsigned int getThirdParameter() const;
    
    /**
     * Set the value that will be returned by the syscall.
     * Invalidates parameters so must be called only after the syscall
     * parameteres have been read.
     * \param ret value that will be returned by the syscall.
     */
    void setReturnValue(unsigned int ret);
    
private:
    unsigned int *registers;
};

/**
 * This class contains information about whether a fault occurred in a process.
 * It is used to terminate processes that fault.
 */
class FaultData
{
public:
    /**
     * Constructor, initializes the object
     */
    FaultData() : id(0) {}
    
    /**
     * Constructor, initializes a FaultData object
     * \param id id of the fault
     * \param pc program counter at the moment of the fault
     * \param arg eventual additional argument, depending on the fault id
     */
    FaultData(int id, unsigned int pc, unsigned int arg=0)
            : id(id), pc(pc), arg(arg) {}
    
    /**
     * \return true if a fault happened within a process
     */
    bool faultHappened() const { return id!=0; }
    
    /**
     * Print information about the occurred fault
     */
    void print() const;
    
private:
    int id; ///< Id of the fault or zero if no faults
    unsigned int pc; ///< Program counter value at the time of the fault
    unsigned int arg;///< Eventual argument, valid only for some id values
};

/**
 * \internal
 * Initializes a ctxsave array when a thread is created.
 * This version is to initialize the userspace context of processes.
 * It is used by the kernel, and should not be used by end users.
 * \param ctxsave a pointer to a field ctxsave inside a Thread class that need
 * to be filled
 * \param pc starting program counter of newly created thread, used to
 * initialize ctxsave
 * \param sp starting stack pointer of newly created thread, used to initialize
 * ctxsave
 * \param argv starting data passed to newly created thread, used to initialize
 * ctxsave
 * \param gotBase base address of the global offset table, for userspace
 * processes
 */
void initCtxsave(unsigned int *ctxsave, void *(*pc)(void *), unsigned int *sp,
        void *argv, unsigned int *gotBase);

/**
 * \internal
 * Cause a supervisor call that will switch the thread back to kernelspace
 * It is used by the kernel, and should not be used by end users.
 */
inline void portableSwitchToUserspace();

#endif //WITH_PROCESSES

/**
 * \internal
 * Called by miosix::start_kernel to handle the architecture-specific part of
 * initialization. It is used by the kernel, and should not be used by end users.
 * It is ensured that the miosix::kernel_started flag false during the execution
 * of this function. Upon return, miosix::kernel_started is set to be true and
 * IRQportableFinishKernelStartup is called immediately.
 * A motivation for this flow could be that it allows running of general purpose
 * driver classes that would be ran either before or after start of the kernel.
 * Probably these drivers may need to disable interrupts using InterruptDisableLock
 * in the case that they are initialized after kernel's startup, while using
 * InterruptDisableLock is error-prone when the kernel_started flag is true and
 * the kernel is not fully started yet.
 */
void IRQportableStartKernel();

/**
 * \internal
 * This function is called right after IRQportableStartKernel by
 * miosix::start_kernel. The miosix::kernel_started is set to true at this
 * stage.
 * A typical behaviour that's expected is to :
 * 1) Enable falut IRQ
 * 2) Enable IRQs
 * 3) miosix::Thread::yield();
 */
void IRQportableFinishKernelStartup();

/**
 * \internal
 * This function disables interrupts.
 * This is used by the kernel to implement disableInterrupts() and
 * enableInterrupts(). You should never need to call these functions directly.
 */
inline void doDisableInterrupts();

/**
 * \internal
 * This function enables interrupts.
 * This is used by the kernel to implement disableInterrupts() and
 * enableInterrupts(). You should never need to call these functions directly.
 */
inline void doEnableInterrupts();

/**
 * \internal
 * This is used by the kernel to implement areInterruptsEnabled()
 * You should never need to call this function directly.
 * \return true if interrupts are enabled
 */
inline bool checkAreInterruptsEnabled();

/**
 * \internal
 * used by the idle thread to put cpu in low power mode
 */
void sleepCpu();

/**
 * \}
 */

} //namespace miosix_private

// This contains the macros and the implementation of inline functions
#include "interfaces-impl/portability_impl.h"
