/***************************************************************************
 *   Copyright (C) 2008, 2009, 2010 by Terraneo Federico                   *
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
#include "interfaces/portability.h"
#include "config/miosix_settings.h"
#include "interrupts.h"

using namespace miosix;

//Look up table used by printUnsignedInt()
static const char hexdigits[]="0123456789abcdef";

/**
 * \internal
 * Used to print an unsigned int in hexadecimal format, and to reboot the system
 * Note that printf/iprintf cannot be used inside an IRQ, so that's why there's
 * this function.
 * \param x number to print
 */
static void printUnsignedInt(unsigned int x)
{
    char result[]="0x........\r\n";
    for(int i=9;i>=2;i--)
    {
        result[i]=hexdigits[x & 0xf];
        x>>=4;
    }
    IRQerrorLog(result);
}

/**
 * \internal
 * Spurious interrupt handler.
 * The LPC2138 datasheet says that spurious interrups can occur, but until now
 * it never happened. If and when spurious interruts will occur, this code will
 * be modified to deal with them. Until then, this code just reboots the system.
 */
void default_IRQ_Routine()
{
    IRQerrorLog("\r\n***Unexpected IRQ\r\n");
    miosix_private::IRQsystemReboot();
}

/**
 * \internal
 * FIQ is currently not used.
 * Prints an error message, and reboots the system.
 * Stack usage is 24 Bytes (measured with watermarking and stack dump)
 * so a 32 byte stack is used (to leave some guard space).
 * If the user wants to use FIQ, it is important to remember to increase the
 * FIQ's stack size, which is defined in miosix.ld
 */
extern "C" void FIQ_Routine() 
{
    IRQerrorLog("\r\n***Unexpected FIQ\r\n");
    miosix_private::IRQsystemReboot();
}

/**
 * \internal
 * This ISR handles Undefined instruction.
 * Prints an error message, showing an address near the instruction that caused
 * the exception. This address together with the map file allows finding the
 * function that caused the exception.
 * Please note that when compiling with some level of optimization, the compiler
 * can inline functions so the address is no longer accurate.
 * Stack usage is 47 Bytes (measured with watermarking and stack dump)
 * so a 48 byte stack is used (stak must be word-aligned).
 */
extern "C" void UNDEF_Routine()
{
    //These two instructions MUST be the first two instructions of the interrupt
    //routine. They store in return_address the pc of the instruction that
    //caused the interrupt.
    register int returnAddress;
    asm volatile("mov	%0, lr" : "=r"(returnAddress));

    IRQerrorLog("\r\n***Unexpected UNDEF @ ");
    printUnsignedInt(returnAddress);
    miosix_private::IRQsystemReboot();
}

/**
 * \internal
 * This ISR handles data abort.
 * Prints an error message, showing an address near the instruction that caused
 * the exception. This address together with the map file allows finding the
 * function that caused the exception.
 * Please note that when compiling with some level of optimization, the compiler
 * can inline functions so the address is no longer accurate.
 * Stack usage is 47 Bytes (measured with watermarking and stack dump)
 * so a 48 byte stack is used (stak must be word-aligned).
 */
extern "C" void DABT_Routine()
{
    //These two instructions MUST be the first two instructions of the interrupt
    //routine. They store in return_address the pc of the instruction that
    //caused the interrupt. (lr has an offset of 8 during a data abort)
    register int returnAddress;
    asm volatile("sub	%0, lr, #8" : "=r"(returnAddress));

    IRQerrorLog("\r\n***Unexpected data abort @ ");
    printUnsignedInt(returnAddress);
    miosix_private::IRQsystemReboot();
}

/**
 * \internal
 * This ISR handles prefetch abort.
 * Prints an error message, showing an address near the instruction that caused
 * the exception. This address together with the map file allows finding the
 * function that caused the exception.
 * Please note that when compiling with some level of optimization, the compiler
 * can inline functions so the address is no longer accurate.
 * Stack usage is 47 Bytes (measured with watermarking and stack dump)
 * so a 48 byte stack is used (stak must be word-aligned).
 */
extern "C" void PABT_Routine()
{
    //These two instructions MUST be the first two instructions of the interrupt
    //routine. They store in return_address the pc of the instruction that
    //caused the interrupt. (lr has an offset of 4 during a data abort)
    register int returnAddress;
    asm volatile("sub	%0, lr, #4" : "=r"(returnAddress));

    IRQerrorLog("\r\n***Unexpected prefetch abort @ ");
    printUnsignedInt(returnAddress);
    miosix_private::IRQsystemReboot();
}
