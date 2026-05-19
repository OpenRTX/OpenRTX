/***************************************************************************
 *   Copyright (C) 2017 by Terraneo Federico                               *
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

#include "stm32_rtc.h"
#include <miosix.h>
#include <sys/ioctl.h>
#include <kernel/scheduler/scheduler.h>

using namespace miosix;

namespace {

//
// class ScopedCnf, RAII to allow writing to the RTC alarm and prescaler.
// NOTE: datasheet says that after CNF bit has been cleared, no write is allowed
// to *any* of the RTC registers, not just PRL,CNT,ALR until RTOFF goes to 1.
// We do not wait until RTOFF is 1 in the destructor for performance reasons,
// so the rest of the code must be careful.
//
class ScopedCnf
{
public:
    ScopedCnf()
    {
        while((RTC->CRL & RTC_CRL_RTOFF)==0) ;
        RTC->CRL=0b11111;
    }
    
    ~ScopedCnf()
    {
        RTC->CRL=0b01111;
    }
};

long long swTime=0;      //64bit software extension of current time in ticks
long long irqTime=0;     //64bit software extension of scheduled irq time in ticks
Thread *waiting=nullptr; //waiting thread

/**
 * \return the hardware counter
 */
inline unsigned int IRQgetHwTick()
{
    unsigned int h1=RTC->CNTH;
    unsigned int l1=RTC->CNTL;
    unsigned int h2=RTC->CNTH;
    if(h1==h2) return (h1<<16) | l1;
    return (h2<<16) | RTC->CNTL;
}

/**
 * \return the time in ticks (hardware part + software extension to 64bits)
 */
long long IRQgetTick()
{
    //Pending bit trick
    unsigned int hwTime=IRQgetHwTick();
    if((RTC->CRL & RTC_CRL_OWF) && IRQgetHwTick()>=hwTime)
        return (swTime + static_cast<long long>(hwTime)) + (1LL<<32);
    return swTime + static_cast<long long>(hwTime);
}

/**
 * Sleep the current thread till the specified time
 * \param tick absolute time in ticks
 * \param dLock used to reenable interrupts while sleeping
 * \return true if the wait time was in the past
 */
bool IRQabsoluteWaitTick(long long tick, FastInterruptDisableLock& dLock)
{
    irqTime=tick;
    unsigned int hwAlarm=(tick & 0xffffffffULL) - (swTime & 0xffffffffULL);
    {
        ScopedCnf cnf;
        RTC->ALRL=hwAlarm;
        RTC->ALRH=hwAlarm>>16;
    }
    //NOTE: We do not wait for the alarm register to be written back to the low
    //frequency domain for performance reasons. The datasheet says it takes
    //at least 3 cycles of the 32KHz clock, but experiments show that it takes
    //from 2 to 3, so perhaps they meant "at most 3". Because of this we
    //consider the time in the past if we are more than 2 ticks of the 16KHz
    //clock (4 ticks of the 32KHz one) in advance. Sleeps less than 122us are
    //thus not supported.
    if(IRQgetTick()>=tick-2) return true;
    waiting=Thread::IRQgetCurrentThread();
    do {
        Thread::IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::yield();
        }
    } while(waiting);
    return false;
}

} //anon namespace

/**
 * RTC interrupt
 */
void __attribute__((naked)) RTC_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z10RTCIrqImplv");
    restoreContext();
}

/**
 * RTC interrupt actual implementation
 */
void __attribute__((used)) RTCIrqImpl()
{
    unsigned int crl=RTC->CRL;
    if(crl & RTC_CRL_OWF)
    {
        RTC->CRL=(RTC->CRL | 0xf) & ~RTC_CRL_OWF;
        swTime+=1LL<<32;
    } else if(crl & RTC_CRL_ALRF) {
        RTC->CRL=(RTC->CRL | 0xf) & ~RTC_CRL_ALRF;
        if(waiting && IRQgetTick()>=irqTime)
        {
            waiting->IRQwakeup();
            if(waiting->IRQgetPriority()>
                Thread::IRQgetCurrentThread()->IRQgetPriority())
                    Scheduler::IRQfindNextThread();
            waiting=nullptr;
        }
    }
}

namespace miosix {

void absoluteDeepSleep(long long int value)
{
    #ifdef RUN_WITH_HSI
    const int wakeupAdvance=3; //waking up takes time
    #else //RUN_WITH_HSI
    const int wakeupAdvance=33; //HSE starup time is 2ms
    #endif //RUN_WITH_HSI
    
    Rtc& rtc=Rtc::instance();
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);

    FastInterruptDisableLock dLock;
    
    //Set alarm and enable EXTI
    long long wkupTick=rtc.tc.ns2tick(value)-wakeupAdvance;
    unsigned int hwAlarm=(wkupTick & 0xffffffffULL) - (swTime & 0xffffffffULL);
    {
        ScopedCnf cnf;
        RTC->ALRL=hwAlarm;
        RTC->ALRH=hwAlarm>>16;
    }
    while((RTC->CRL & RTC_CRL_RTOFF)==0) ;
    
    
    EXTI->RTSR |= EXTI_RTSR_TR17;
    EXTI->EMR  |= EXTI_EMR_MR17; //enable event for wakeup
    
    long long tick=IRQgetTick();
    while(tick<wkupTick)
    {
        PWR->CR |= PWR_CR_LPDS;
        SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
        // Using WFE instead of WFI because if while we are with interrupts
        // disabled an interrupt (such as the tick interrupt) occurs, it
        // remains pending and the WFI becomes a nop, and the device never goes
        // in sleep mode. WFE events are latched in a separate pending register
        // so interrupts do not interfere with them       
        __WFE();
        SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
        PWR->CR &= ~PWR_CR_LPDS;
        
        #ifndef SYSCLK_FREQ_24MHz
        #error TODO: support more PLL frequencies
        #endif
        //STOP mode resets the clock to the HSI 8MHz, so restore the 24MHz clock
        #ifndef RUN_WITH_HSI
        RCC->CR |= RCC_CR_HSEON;
        while((RCC->CR & RCC_CR_HSERDY)==0) ;
        //PLL = (HSE / 2) * 6 = 24 MHz
        RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);
        RCC->CFGR |=   RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL6;
        #else //RUN_WITH_HSI
        //PLL = (HSI / 2) * 6 = 24 MHz
        RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);
        RCC->CFGR |=                     RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL6;
        #endif //RUN_WITH_HSI        
        RCC->CR |= RCC_CR_PLLON;
        while((RCC->CR & RCC_CR_PLLRDY)==0) ;
        RCC->CFGR &= ~RCC_CFGR_SW;
        RCC->CFGR |= RCC_CFGR_SW_PLL;    
        while((RCC->CFGR & RCC_CFGR_SWS)!=0x08) ;
        
        //Wait RSF
        RTC->CRL=(RTC->CRL | 0xf) & ~RTC_CRL_RSF;
        while((RTC->CRL & RTC_CRL_RSF)==0) ;
        
        //Clear IRQ
        unsigned int crl=RTC->CRL;
        //NOTE: ST docs say OWF, ALRF are not updated in deep sleep. So, to
        //detect counter ovreflow we check for oldTick>newTick. The check for
        //flags is still there in case overflow/alarm occurs before/after sleep
        if(crl & RTC_CRL_OWF || tick>IRQgetTick())
        {
            RTC->CRL=(RTC->CRL | 0xf) & ~RTC_CRL_OWF;
            swTime+=1LL<<32;
        } else if(crl & RTC_CRL_ALRF) {
            RTC->CRL=(RTC->CRL | 0xf) & ~RTC_CRL_ALRF;
        }
        
        //Update tick, done after checking IRQ flags to avoid race condition
        tick=IRQgetTick();
    }
    
    EXTI->EMR &=~ EXTI_EMR_MR17; //disable event for wakeup
    
    wkupTick+=wakeupAdvance;
    
    //NOTE: if we use the HSE we may spin for a while, but adding a
    //IRQabsoluteWaitTick here makes this function wakeup too late
    while(IRQgetTick()<wkupTick) ;
}

//
// class Rtc
//

Rtc& Rtc::instance()
{
    static Rtc singleton;
    return singleton;
}

long long Rtc::getValue() const
{
    //Function takes ~170 clock cycles ~60 cycles IRQgetTick, ~96 cycles tick2ns
    long long tick;
    {
        FastInterruptDisableLock dLock;
        tick=IRQgetTick();
    }
    //tick2ns is reentrant, so can be called with interrupt enabled
    return tc.tick2ns(tick);
}

long long Rtc::IRQgetValue() const
{
    return tc.tick2ns(IRQgetTick());
}

// May not work due to the way hwAlarm is computed in sleep functions
// void Rtc::setValue(long long value)
// {
//     FastInterruptDisableLock dLock;
//     swTime=tc.ns2tick(value)-IRQgetHwTick();
// }

void Rtc::wait(long long value)
{
    FastInterruptDisableLock dLock;
    IRQabsoluteWaitTick(IRQgetTick()+tc.ns2tick(value),dLock);
}

bool Rtc::absoluteWait(long long value)
{
    FastInterruptDisableLock dLock;
    return IRQabsoluteWaitTick(tc.ns2tick(value),dLock);
}

Rtc::Rtc() : tc(getTickFrequency())
{
    {
        FastInterruptDisableLock dLock;
        RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
        PWR->CR |= PWR_CR_DBP;
        RCC->BDCR=RCC_BDCR_RTCEN       //RTC enabled
                | RCC_BDCR_LSEON       //External 32KHz oscillator enabled
                | RCC_BDCR_RTCSEL_0;   //Select LSE as clock source for RTC
        RCC_SYNC();
        #ifdef RTC_CLKOUT_ENABLE
        BKP->RTCCR=BKP_RTCCR_CCO;      //Output RTC clock/64 on pin
        #endif
    }
    while((RCC->BDCR & RCC_BDCR_LSERDY)==0) ; //Wait for LSE to start
    
    RTC->CRH=RTC_CRH_OWIE | RTC_CRH_ALRIE;
    {
        ScopedCnf cnf;
        RTC->PRLH=0;
        RTC->PRLL=1;
    }
    NVIC_SetPriority(RTC_IRQn,5);
    NVIC_EnableIRQ(RTC_IRQn);
}

} //namespace miosix
