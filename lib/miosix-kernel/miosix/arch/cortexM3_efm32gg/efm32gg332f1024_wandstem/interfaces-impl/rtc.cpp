/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
 *   Copyright (C) 2013, 2014 by Terraneo Federico and Luigi Rinaldi       *
 *   Copyright (C) 2015, 2016 by Terraneo Federico, Luigi Rinaldi and      *
 *   Silvano Seva                                                          *
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

#include "rtc.h"
#include <miosix.h>
#include <kernel/scheduler/scheduler.h>
#include "gpioirq.h"
#include "config/miosix_settings.h"
#include "hrtb.h"

using namespace miosix;

//enum class WaitResult
//{
//    WAKEUP_IN_THE_PAST,
//    WAIT_COMPLETED,
//    EVENT
//};

const unsigned int timerBits=24;
const unsigned long long overflowIncrement=(1LL<<timerBits);
const unsigned long long lowerMask=overflowIncrement-1;

static long long swCounter=0;         ///< RTC software counter in ticks
static unsigned int lastHwCounter=0;  ///< variable for evaluating overflows

static Thread *rtcWaiting=nullptr;    ///< Thread waiting for the RTC irq
static bool rtcTriggerEnable=false;   ///< If true COMP0 causes trigger
static bool eventOccurred=false;      ///< Set by the event pin irq
static long long timestampEvent=0;    ///< input capture timestamp in ticks

/**
 * Fast (inline) function for reading the RTC
 * \return the RTC value in ticks, extended in software to 64 bits
 */
static inline long long IRQreadRtc()
{
    //PENDING BIT TRICK
    unsigned int counter=RTC->CNT;
    if((RTC->IF & _RTC_IFC_OF_MASK) && RTC->CNT>=counter)
        return (swCounter | static_cast<long long>(counter)) + overflowIncrement;
    return swCounter | static_cast<long long>(counter);
}

/**
 * Common part of all wait functions
 * \param value absolute time point when the wait has to end
 * \param eventSensitive if true, return prematurely if an event occurs
 * \return the condition that caused the function to return
 */
static WaitResult waitImpl(long long value, bool eventSensitive)
{
    auto eventPin=transceiver::excChB::getPin();
    //EFM32 compare channels trigger 1 tick late (undocumented quirk)
    RTC->COMP0=(value-1) & 0xffffff;
    while(RTC->SYNCBUSY & RTC_SYNCBUSY_COMP0) ;
    
    FastInterruptDisableLock dLock;
    //NOTE: this is very important, enabling the interrupt without clearing the
    //interrupt flag causes the function to return prematurely, sometimes
    RTC->IFC=RTC_IFC_COMP0;
    RTC->IEN |= RTC_IEN_COMP0;
    
    if(eventSensitive)
    {
        //To avoid race condition, first enable irq, then check for event
        IRQenableGpioIrq(eventPin);
        //Event occurred. Note that here we assume as event model a device
        //that raises the pin and holds it until some action such as writing
        //to its registers to clear the pin, as is the case with the cc2520
        //so if the event occurred in the past and we were waiting for that
        //event, the pin is surely high. A device that raises the pin just
        //briefly would cause a race condition
        if(miosix::transceiver::excChB::value()==1)
        {
            IRQdisableGpioIrq(eventPin);
            RTC->IFC=RTC_IFC_COMP0;
            RTC->IEN &= ~RTC_IEN_COMP0;
            return WaitResult::EVENT;
        }
        eventOccurred=false;
    }
    
    //NOTE: the corner case where the wakeup is now is considered "in the past"
    if(value<=IRQreadRtc())
    {
        if(eventSensitive) IRQdisableGpioIrq(eventPin);
        RTC->IFC=RTC_IFC_COMP0;
        RTC->IEN &= ~RTC_IEN_COMP0;
        return WaitResult::WAKEUP_IN_THE_PAST;
    }
    
    do {
        rtcWaiting=Thread::IRQgetCurrentThread();
        Thread::IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::yield();
        }
        //The readRtc() check in the while is for waits past one RTC period
    } while(rtcWaiting && value>IRQreadRtc());
    RTC->IEN &= ~RTC_IEN_COMP0;
    if(eventSensitive)
    {
        IRQdisableGpioIrq(eventPin);
        if(eventOccurred) return WaitResult::EVENT;
    }
    return WaitResult::WAIT_COMPLETED;
}

/**
 * RTC interrupt
 */
void __attribute__((naked)) RTC_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z14RTChandlerImplv");
    restoreContext();
}

/**
 * RTC interrupt actual implementation
 */
void __attribute__((used)) RTChandlerImpl()
{
    if(RTC->IF & RTC_IF_OF){
        RTC->IFC=RTC_IFC_OF;
        swCounter+=overflowIncrement;
    }
    
    if(RTC->IF & RTC_IF_COMP0)
    {
        RTC->IFC=RTC_IFC_COMP0;
        
        if(rtcTriggerEnable)
        {
            //High time is around 120ns
            transceiver::stxon::high();
            rtcTriggerEnable=false;
            transceiver::stxon::low();
        }
    
        if(rtcWaiting)
        {
            rtcWaiting->IRQwakeup();
            if(rtcWaiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
                Scheduler::IRQfindNextThread();
            rtcWaiting=nullptr;
        }
    }
    
    if(RTC->IF & RTC_IF_COMP1){
        RTC->IFC=RTC_IFC_COMP1;
    }
}

/**
 * Event timestamping pin interrupt actual implementation
 */
void GPIO8Handler()
{   
    timestampEvent=IRQreadRtc();
    eventOccurred=true;
    
    if(!rtcWaiting) return;
    rtcWaiting->IRQwakeup();
    if(rtcWaiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
    rtcWaiting=nullptr;
}

namespace miosix {

//
// class Rtc
//

Rtc& Rtc::instance()
{
    static Rtc timer;
    return timer;
}

long long Rtc::getValue() const
{
    //readRtc() is not reentrant, and is also called in the GPIO timestamp irq
    FastInterruptDisableLock dLock;
    return IRQreadRtc();
}

long long int Rtc::IRQgetValue() const
{
    return IRQreadRtc();
}

void Rtc::setValue(long long value)
{
    //Stop timer and wait for it to be stopped
    RTC->CTRL=0;
    unsigned int hwCounter=value & 0x0000000000ffffffull;
    while(RTC->SYNCBUSY & RTC_SYNCBUSY_CTRL) ;
    
    RTC->CNT=hwCounter;
    
    //Restart timer as soon as possible
    RTC->CTRL=RTC_CTRL_EN;
    swCounter=value & 0xffffffffff000000ull;
    lastHwCounter=hwCounter;
    while(RTC->SYNCBUSY & RTC_SYNCBUSY_CTRL) ;
}

void Rtc::wait(long long value)
{
    waitImpl(getValue()+value,false);
}

bool Rtc::absoluteWait(long long value)
{
    return waitImpl(value,false)==WaitResult::WAKEUP_IN_THE_PAST;
}

bool Rtc::absoluteWaitTrigger(long long value)
{
    rtcTriggerEnable=true;
    bool result=waitImpl(value,false)==WaitResult::WAKEUP_IN_THE_PAST;
    rtcTriggerEnable=false;
    return result;
}

bool Rtc::waitTimeoutOrEvent(long long value)
{
    return waitImpl(getValue()+value,true)!=WaitResult::EVENT;
}

bool Rtc::absoluteWaitTimeoutOrEvent(long long value)
{
    return waitImpl(value,true)!=WaitResult::EVENT;
}

long long Rtc::getExtEventTimestamp(Correct c) const
{
    return timestampEvent;
}

long long int Rtc::tick2ns(long long int tick)
{
    return tc.tick2ns(tick);
}

long long int Rtc::ns2tick(long long int ns)
{
    return tc.ns2tick(ns);
}

unsigned int Rtc::getTickFrequency() const
{
    return frequency;
}

Rtc::Rtc() : tc(frequency)
{
    FastInterruptDisableLock dLock;
    
    //
    // Configure timer
    //
    
    //The LFXO is already started by the BSP
    CMU->HFCORECLKEN0 |= CMU_HFCORECLKEN0_LE; //Enable clock to LE peripherals
    CMU->LFACLKEN0 |= CMU_LFACLKEN0_RTC;
    while(CMU->SYNCBUSY & CMU_SYNCBUSY_LFACLKEN0) ;
    
    RTC->CNT=0;
    
    RTC->CTRL=RTC_CTRL_EN;
    while(RTC->SYNCBUSY & RTC_SYNCBUSY_CTRL) ;
    
    //In the EFM32GG332F1024 the RTC has two compare channels, used in this way:
    //COMP0 -> used for wait and trigger
    //COMP1 -> reserved for VHT resync and Power manager
    //NOTE: interrupt not yet enabled as we're not setting RTC->IEN
    NVIC_EnableIRQ(RTC_IRQn);
    NVIC_SetPriority(RTC_IRQn,7); // 0 is the higest priority, 15 il the lowest
    
    RTC->IEN |= RTC_IEN_OF;
    
    //
    // Configure the GPIO interrupt used for packet reception timestamping
    // (at the RTC resolution using a hardware input capture/output compare
    // channel isn't necessary, as one RTC tick is more than 1400 CPU cycles)
    //
    //Not more needed
    //registerGpioIrq(transceiver::excChB::getPin(),GpioIrqEdge::RISING,GPIO8Handler);
}

} //namespace miosix
