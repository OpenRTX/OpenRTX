/***************************************************************************
 *   Copyright (C) 2016 by Terraneo Federico                               *
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

#include "gpioirq.h"
#include <stdexcept>
#include <kernel/scheduler/scheduler.h>

using namespace std;

static function<void ()> callbacks[16]; ///< Registered callbacks

/**
 * Gpio interrupt for even pin numbers
 */
void __attribute__((naked)) GPIO_EVEN_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z19GPIOEvenHandlerImplv");
    restoreContext();
}

/**
 * Gpio interrupt for odd pin numbers
 */
void __attribute__((naked)) GPIO_ODD_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z18GPIOOddHandlerImplv");
    restoreContext();
}

/**
 * Gpio interrupt for even pin numbers actual implementation
 */
void __attribute__((used)) GPIOEvenHandlerImpl()
{
    for(int i=0;i<16;i+=2)
    {
        if((GPIO->IF & (1<<i))==0) continue;
        GPIO->IFC=1<<i;
        if(callbacks[i]) callbacks[i]();
    }
}

/**
 * Gpio interrupt for odd pin numbers actual implementation
 */
void __attribute__((used)) GPIOOddHandlerImpl()
{
    for(int i=1;i<16;i+=2)
    {
        if((GPIO->IF & (1<<i))==0) continue;
        GPIO->IFC=1<<i;
        if(callbacks[i]) callbacks[i]();
    }
}


namespace miosix {

void registerGpioIrq(GpioPin pin, GpioIrqEdge edge, function<void ()> callback)
{
    unsigned int port=pin.getPort(), number=pin.getNumber();
    if(port>5) throw range_error("Port out of range");
    if(number>15) throw range_error("Pin number out of range");
    
    bool failed=false;
    {
        FastInterruptDisableLock dLock;
        static bool first=false;
        if(first==false)
        {
            first=true;
            GPIO->INSENSE |= GPIO_INSENSE_INT | GPIO_INSENSE_PRS;
            NVIC_EnableIRQ(GPIO_EVEN_IRQn);
            NVIC_SetPriority(GPIO_EVEN_IRQn,10); //Low priority
            NVIC_EnableIRQ(GPIO_ODD_IRQn);
            NVIC_SetPriority(GPIO_ODD_IRQn,10); //Low priority
        }
        
        if(callbacks[number])
        {
            failed=true;
        } else {
            //Swap is nothrow guaranteed, so it can't call unexpected code
            //with irq disabled
            callbacks[number].swap(callback);
            if(number<8)
            {
                GPIO->EXTIPSELL &= ~(0b111<<(4*number));
                GPIO->EXTIPSELL |= port<<(4*number);
            } else {
                GPIO->EXTIPSELH &= ~(0b111<<(4*(number-8)));
                GPIO->EXTIPSELH |= port<<(4*(number-8));
            }
            if(edge==GpioIrqEdge::RISING || edge==GpioIrqEdge::BOTH)
                GPIO->EXTIRISE |= (1<<number);
            else GPIO->EXTIRISE &= ~(1<<number);
            if(edge==GpioIrqEdge::FALLING || edge==GpioIrqEdge::BOTH)
                GPIO->EXTIFALL |= (1<<number);
            else GPIO->EXTIFALL &= ~(1<<number);
        }
    }
    if(failed) throw runtime_error("Pin number already in use");
}

void enableGpioIrq(GpioPin pin)
{
    bool ok;
    {
        FastInterruptDisableLock dLock;
        ok=IRQenableGpioIrq(pin);
    }
    if(ok==false) throw runtime_error("Pin number not in use");
}

void disableGpioIrq(GpioPin pin)
{
    bool ok;
    {
        FastInterruptDisableLock dLock;
        ok=IRQdisableGpioIrq(pin);
    }
    if(ok==false) throw runtime_error("Pin number not in use");
}

bool IRQenableGpioIrq(GpioPin pin)
{
    unsigned int number=pin.getNumber();
    if(number>15 || !callbacks[number]) return false;
    GPIO->IFC=1<<number;
    GPIO->IEN |= (1<<number);
    return true;
}

bool IRQdisableGpioIrq(GpioPin pin)
{
    unsigned int number=pin.getNumber();
    if(number>15 || !callbacks[number]) return false;
    GPIO->IEN &= ~(1<<number);
    GPIO->IFC=1<<number;
    return true;
}

void unregisterGpioIrq(GpioPin pin)
{
    unsigned int number=pin.getNumber();
    if(number>15) throw range_error("Pin number out of range");
    function<void ()> empty;
    {
        FastInterruptDisableLock dLock;
        IRQdisableGpioIrq(pin);
        //Swap is nothrow guaranteed, so it can't call unexpected code
        //with irq disabled
        callbacks[number].swap(empty);
    }
}

} //namespace miosix
