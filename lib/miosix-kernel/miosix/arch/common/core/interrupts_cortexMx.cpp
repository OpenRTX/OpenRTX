/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012, 2013, 2014 by Terraneo Federico       *
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

#include "kernel/logging.h"
#include "kernel/kernel.h"
#include "config/miosix_settings.h"
#include "interfaces/portability.h"
#include "interfaces/arch_registers.h"
#include "interrupts.h"

using namespace miosix;

#ifdef WITH_ERRLOG

/**
 * \internal
 * Used to print an unsigned int in hexadecimal format, and to reboot the system
 * Note that printf/iprintf cannot be used inside an IRQ, so that's why there's
 * this function.
 * \param x number to print
 */
static void printUnsignedInt(unsigned int x)
{
    static const char hexdigits[]="0123456789abcdef";
    char result[]="0x........\r\n";
    for(int i=9;i>=2;i--)
    {
        result[i]=hexdigits[x & 0xf];
        x>>=4;
    }
    IRQerrorLog(result);
}

#endif //WITH_ERRLOG

#if defined(WITH_PROCESSES) || defined(WITH_ERRLOG)

/**
 * \internal
 * \return the program counter of the thread that was running when the exception
 * occurred.
 */
static unsigned int getProgramCounter()
{
    register unsigned int result;
    // Get program counter when the exception was thrown from stack frame
    asm volatile("mrs   %0,  psp    \n\t"
                 "add   %0, %0, #24 \n\t"
                 "ldr   %0, [%0]    \n\t":"=r"(result));
    return result;
}

#endif //WITH_PROCESSES || WITH_ERRLOG

void NMI_Handler()
{
    IRQerrorLog("\r\n***Unexpected NMI\r\n");
    miosix_private::IRQsystemReboot();
}

void __attribute__((naked)) HardFault_Handler()
{
    saveContext();
    //Call HardFault_impl(). Name is a C++ mangled name.
    asm volatile("bl _Z14HardFault_implv");
    restoreContext();
}

void __attribute__((noinline)) HardFault_impl()
{
    #ifdef WITH_PROCESSES
    if(miosix::Thread::IRQreportFault(miosix_private::FaultData(
        HARDFAULT,getProgramCounter()))) return;
    #endif //WITH_PROCESSES
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected HardFault @ ");
    printUnsignedInt(getProgramCounter());
    #if !defined(_ARCH_CORTEXM0_STM32F0) && !defined(_ARCH_CORTEXM0_STM32G0) && !defined(_ARCH_CORTEXM0_STM32L0)
    unsigned int hfsr=SCB->HFSR;
    if(hfsr & 0x40000000) //SCB_HFSR_FORCED
        IRQerrorLog("Fault escalation occurred\r\n");
    if(hfsr & 0x00000002) //SCB_HFSR_VECTTBL
        IRQerrorLog("A BusFault occurred during a vector table read\r\n");
    #endif // !_ARCH_CORTEXM0_STM32F0 && !_ARCH_CORTEXM0_STM32G0 && !_ARCH_CORTEXM0_STM32L0
    #endif //WITH_ERRLOG
    miosix_private::IRQsystemReboot();
}

// Cortex M0/M0+ architecture does not have the interrupts handled by code
// below this point
#if !defined(_ARCH_CORTEXM0_STM32F0) && !defined(_ARCH_CORTEXM0_STM32G0) && !defined(_ARCH_CORTEXM0_STM32L0)

void __attribute__((naked)) MemManage_Handler()
{
    saveContext();
    //Call MemManage_impl(). Name is a C++ mangled name.
    asm volatile("bl _Z14MemManage_implv");
    restoreContext();
}

void __attribute__((noinline)) MemManage_impl()
{
    #if defined(WITH_PROCESSES) || defined(WITH_ERRLOG)
    unsigned int cfsr=SCB->CFSR;
    #endif //WITH_PROCESSES || WITH_ERRLOG
    #ifdef WITH_PROCESSES
    int id, arg=0;
    if(cfsr & 0x00000001) id=MP_XN;
    else if(cfsr & 0x00000080) { id=MP; arg=SCB->MMFAR; }
    else id=MP_NOADDR;
    if(miosix::Thread::IRQreportFault(miosix_private::FaultData(
        id,getProgramCounter(),arg)))
    {
        SCB->SHCSR &= ~(1<<13); //Clear MEMFAULTPENDED bit
        return;
    }
    #endif //WITH_PROCESSES
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected MemManage @ ");
    printUnsignedInt(getProgramCounter());
    if(cfsr & 0x00000080) //SCB_CFSR_MMARVALID
    {
        IRQerrorLog("Fault caused by attempted access to ");
        printUnsignedInt(SCB->MMFAR);
    } else IRQerrorLog("The address that caused the fault is missing\r\n");
    if(cfsr & 0x00000010) //SCB_CFSR_MSTKERR
        IRQerrorLog("Fault occurred during exception stacking\r\n");
    if(cfsr & 0x00000008) //SCB_CFSR_MUNSTKERR
        IRQerrorLog("Fault occurred during exception unstacking\r\n");
    if(cfsr & 0x00000002) //SCB_CFSR_DACCVIOL
        IRQerrorLog("Fault was caused by invalid PC\r\n");
    if(cfsr & 0x00000001) //SCB_CFSR_IACCVIOL
        IRQerrorLog("Fault was caused by attempted execution from XN area\r\n");
    #endif //WITH_ERRLOG
    miosix_private::IRQsystemReboot();
}

void __attribute__((naked)) BusFault_Handler()
{
    saveContext();
    //Call BusFault_impl(). Name is a C++ mangled name.
    asm volatile("bl _Z13BusFault_implv");
    restoreContext();
}

void __attribute__((noinline)) BusFault_impl()
{
    #if defined(WITH_PROCESSES) || defined(WITH_ERRLOG)
    unsigned int cfsr=SCB->CFSR;
    #endif //WITH_PROCESSES || WITH_ERRLOG
    #ifdef WITH_PROCESSES
    int id, arg=0;
    if(cfsr & 0x00008000) { id=BF; arg=SCB->BFAR; }
    else id=BF_NOADDR;
    if(miosix::Thread::IRQreportFault(miosix_private::FaultData(
        id,getProgramCounter(),arg)))
    {
        SCB->SHCSR &= ~(1<<14); //Clear BUSFAULTPENDED bit
        return;
    }
    #endif //WITH_PROCESSES
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected BusFault @ ");
    printUnsignedInt(getProgramCounter());
    if(cfsr & 0x00008000) //SCB_CFSR_BFARVALID
    {
        IRQerrorLog("Fault caused by attempted access to ");
        printUnsignedInt(SCB->BFAR);
    } else IRQerrorLog("The address that caused the fault is missing\r\n");
    if(cfsr & 0x00001000) //SCB_CFSR_STKERR
        IRQerrorLog("Fault occurred during exception stacking\r\n");
    if(cfsr & 0x00000800) //SCB_CFSR_UNSTKERR
        IRQerrorLog("Fault occurred during exception unstacking\r\n");
    if(cfsr & 0x00000400) //SCB_CFSR_IMPRECISERR
        IRQerrorLog("Fault is imprecise\r\n");
    if(cfsr & 0x00000200) //SCB_CFSR_PRECISERR
        IRQerrorLog("Fault is precise\r\n");
    if(cfsr & 0x00000100) //SCB_CFSR_IBUSERR
        IRQerrorLog("Fault happened during instruction fetch\r\n");
    #endif //WITH_ERRLOG
    miosix_private::IRQsystemReboot();
}

void __attribute__((naked)) UsageFault_Handler()
{
    saveContext();
    //Call UsageFault_impl(). Name is a C++ mangled name.
    asm volatile("bl _Z15UsageFault_implv");
    restoreContext();
}

void __attribute__((noinline)) UsageFault_impl()
{
    #if defined(WITH_PROCESSES) || defined(WITH_ERRLOG)
    unsigned int cfsr=SCB->CFSR;
    #endif //WITH_PROCESSES || WITH_ERRLOG
    #ifdef WITH_PROCESSES
    int id;
    if(cfsr & 0x02000000) id=UF_DIVZERO;
    else if(cfsr & 0x01000000) id=UF_UNALIGNED;
    else if(cfsr & 0x00080000) id=UF_COPROC;
    else if(cfsr & 0x00040000) id=UF_EXCRET;
    else if(cfsr & 0x00020000) id=UF_EPSR;
    else if(cfsr & 0x00010000) id=UF_UNDEF;
    else id=UF_UNEXP;
    if(miosix::Thread::IRQreportFault(miosix_private::FaultData(
        id,getProgramCounter())))
    {
        SCB->SHCSR &= ~(1<<12); //Clear USGFAULTPENDED bit
        return;
    }
    #endif //WITH_PROCESSES
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected UsageFault @ ");
    printUnsignedInt(getProgramCounter());
    if(cfsr & 0x02000000) //SCB_CFSR_DIVBYZERO
        IRQerrorLog("Divide by zero\r\n");
    if(cfsr & 0x01000000) //SCB_CFSR_UNALIGNED
        IRQerrorLog("Unaligned memory access\r\n");
    if(cfsr & 0x00080000) //SCB_CFSR_NOCP
        IRQerrorLog("Attempted coprocessor access\r\n");
    if(cfsr & 0x00040000) //SCB_CFSR_INVPC
        IRQerrorLog("EXC_RETURN not expected now\r\n");
    if(cfsr & 0x00020000) //SCB_CFSR_INVSTATE
        IRQerrorLog("Invalid EPSR usage\r\n");
    if(cfsr & 0x00010000) //SCB_CFSR_UNDEFINSTR
        IRQerrorLog("Undefined instruction\r\n");
    #endif //WITH_ERRLOG
    miosix_private::IRQsystemReboot();
}

void DebugMon_Handler()
{
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected DebugMon @ ");
    printUnsignedInt(getProgramCounter());
    #endif //WITH_ERRLOG
    miosix_private::IRQsystemReboot();
}

#endif // !_ARCH_CORTEXM0_STM32F0 && !_ARCH_CORTEXM0_STM32G0 && !_ARCH_CORTEXM0_STM32L0

void PendSV_Handler()
{
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected PendSV @ ");
    printUnsignedInt(getProgramCounter());
    #endif //WITH_ERRLOG
    miosix_private::IRQsystemReboot();
}

void unexpectedInterrupt()
{
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected Peripheral interrupt\r\n");
    #endif //WITH_ERRLOG
    miosix_private::IRQsystemReboot();
}
