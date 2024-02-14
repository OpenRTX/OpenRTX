 /***************************************************************************
 *   Copyright (C) 2014 by Terraneo Federico                               *
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
 
#include "servo_stm32.h"
#include "kernel/scheduler/scheduler.h"
#include <algorithm>
#include <cstdio>
#include <cmath>

using namespace std;
using namespace miosix;

typedef Gpio<GPIOB_BASE,6> servo1out;
typedef Gpio<GPIOB_BASE,7> servo2out;
typedef Gpio<GPIOB_BASE,8> servo3out;
typedef Gpio<GPIOB_BASE,9> servo4out;
static Thread *waiting=0;

/**
 * Timer 4 interrupt handler actual implementation
 */
void __attribute__((used)) tim4impl()
{
    TIM4->SR=0; //Clear interrupt flag
    if(waiting==0) return;
    waiting->IRQwakeup();
    if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
    waiting=0;
}

/**
 * Timer 4 interrupt handler
 */
void __attribute__((naked)) TIM4_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z8tim4implv");
    restoreContext();
}

namespace miosix {

/* TODO: find a better place for this */
unsigned int divideRoundToNearest(unsigned int a, unsigned int b)
{
    const unsigned int quot=2*a/b;
    return quot/2 + (quot & 1);
}

//
// class SynchronizedServo
//

SynchronizedServo& SynchronizedServo::instance()
{
    static SynchronizedServo singleton;
    return singleton;
}

void SynchronizedServo::enable(int channel)
{
    Lock<FastMutex> l(mutex);
    if(status!=STOPPED) return; // If timer enabled ignore the call
    
    {
        FastInterruptDisableLock dLock;
        // Calling the mode() function on a GPIO is subject to race conditions
        // between threads on the STM32, so we disable interrupts
        switch(channel)
        {
            case 0:
                TIM4->CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE;
                TIM4->CCER |= TIM_CCER_CC1E;
                #ifndef _ARCH_CORTEXM3_STM32F1 //Only stm32f2 and stm32f4 have it
                servo1out::alternateFunction(2);
                #endif //_ARCH_CORTEXM3_STM32F1
                servo1out::mode(Mode::ALTERNATE);
                break;
            case 1:
                TIM4->CCMR1 |= TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2PE;
                TIM4->CCER |= TIM_CCER_CC2E;
                #ifndef _ARCH_CORTEXM3_STM32F1 //Only stm32f2 and stm32f4 have it
                servo2out::alternateFunction(2);
                #endif //_ARCH_CORTEXM3_STM32F1
                servo2out::mode(Mode::ALTERNATE);
                break;
            case 2:
                TIM4->CCMR2 |= TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3PE;
                TIM4->CCER |= TIM_CCER_CC3E;
                #ifndef _ARCH_CORTEXM3_STM32F1 //Only stm32f2 and stm32f4 have it
                servo3out::alternateFunction(2);
                #endif //_ARCH_CORTEXM3_STM32F1
                servo3out::mode(Mode::ALTERNATE);
                break;
            case 3:
                TIM4->CCMR2 |= TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4PE;
                TIM4->CCER |= TIM_CCER_CC4E;
                #ifndef _ARCH_CORTEXM3_STM32F1 //Only stm32f2 and stm32f4 have it
                servo4out::alternateFunction(2);
                #endif //_ARCH_CORTEXM3_STM32F1
                servo4out::mode(Mode::ALTERNATE);
                break;
        }
    }
}

void SynchronizedServo::disable(int channel)
{
    Lock<FastMutex> l(mutex);
    if(status!=STOPPED) return; // If timer enabled ignore the call
    
    {
        FastInterruptDisableLock dLock;
        // Calling the mode() function on a GPIO is subject to race conditions
        // between threads on the STM32, so we disable interrupts
        switch(channel)
        {
            case 0:
                servo1out::mode(Mode::INPUT);
                TIM4->CCER &= ~TIM_CCER_CC1E;
                break;
            case 1:
                servo2out::mode(Mode::INPUT);
                TIM4->CCER &= ~TIM_CCER_CC2E;
                break;
            case 2:
                servo3out::mode(Mode::INPUT);
                TIM4->CCER &= ~TIM_CCER_CC3E;
                break;
            case 3:
                servo4out::mode(Mode::INPUT);
                TIM4->CCER &= ~TIM_CCER_CC4E;
                break;
        }
    }
}

void SynchronizedServo::setPosition(int channel, float value)
{
    Lock<FastMutex> l(mutex);
    if(status!=STARTED) return; // If timer disabled ignore the call
    
    value=min(1.0f,max(0.0f,value));
    switch(channel)
    {
        case 0:
            TIM4->CCR1=a*value+b;
            break;
        case 1:
            TIM4->CCR2=a*value+b;
            break;
        case 2:
            TIM4->CCR3=a*value+b;
            break;
        case 3:
            TIM4->CCR4=a*value+b;
            break;
    }
}

float SynchronizedServo::getPosition(int channel)
{
    switch(channel)
    {
        case 0:
            return TIM4->CCR1==0 ? NAN : TIM4->CCR1/a-b;
        case 1:
            return TIM4->CCR2==0 ? NAN : TIM4->CCR2/a-b;
        case 2:
            return TIM4->CCR3==0 ? NAN : TIM4->CCR3/a-b;
        case 3:
            return TIM4->CCR4==0 ? NAN : TIM4->CCR4/a-b;
        default:
            return NAN;
    }
}

void SynchronizedServo::start()
{
    Lock<FastMutex> l(mutex);
    if(status!=STOPPED) return; // If timer enabled ignore the call
    
    // While status is starting neither memeber function callable with timer
    // started nor stopped are allowed
    status=STARTED;
    TIM4->CNT=0;
    TIM4->EGR=TIM_EGR_UG;
    TIM4->CR1=TIM_CR1_CEN;
}

void SynchronizedServo::stop()
{
    Lock<FastMutex> l(mutex);
    if(status!=STARTED) return; // If timer disabled ignore the call
    
    status=STOPPED;
    // Stopping the timer is a bit difficult because we don't want to
    // accidentally create glitches on the outputs which may turn the servos
    // to random positions. So, we set all PWM outputs to 0, then wait until the
    // end of the period, and then disable the timer
    TIM4->CCR1=0;
    TIM4->CCR2=0;
    TIM4->CCR3=0;
    TIM4->CCR4=0;
    {
        FastInterruptDisableLock dLock;
        // Wakeup an eventual thread waiting on waitForCycleBegin()
        if(waiting) waiting->IRQwakeup();
        IRQwaitForTimerOverflow(dLock);
    }
    TIM4->CR1=0;
}

bool SynchronizedServo::waitForCycleBegin()
{
    // No need to lock the mutex because disabling interrupts is enough to avoid
    // race conditions. Also, locking the mutex here would prevent other threads
    // from calling other member functions of this class
    FastInterruptDisableLock dLock;
    if(status!=STARTED) return true;
    IRQwaitForTimerOverflow(dLock);
    return status!=STARTED;
}

void SynchronizedServo::setFrequency(unsigned int frequency)
{
    Lock<FastMutex> l(mutex);
    if(status!=STOPPED) return; // If timer enabled ignore the call
    
    TIM4->PSC=divideRoundToNearest(getPrescalerInputFrequency(),frequency*65536)-1;
    precomputeCoefficients();
}

float SynchronizedServo::getFrequency() const
{
    float prescalerFreq=getPrescalerInputFrequency();
    return prescalerFreq/((TIM4->PSC+1)*65536);
}

void SynchronizedServo::setMinPulseWidth(float minPulse)
{
    Lock<FastMutex> l(mutex);
    if(status!=STOPPED) return; // If timer enabled ignore the call
    
    minWidth=1e-6f*min(1300.0f,max(500.0f,minPulse));
    precomputeCoefficients();
}

void SynchronizedServo::setMaxPulseWidth(float maxPulse)
{
    Lock<FastMutex> l(mutex);
    if(status!=STOPPED) return; // If timer enabled ignore the call
    
    maxWidth=1e-6f*min(2500.0f,max(1700.0f,maxPulse));
    precomputeCoefficients();
}

SynchronizedServo::SynchronizedServo() : status(STOPPED)
{
    {
        FastInterruptDisableLock dLock;
        // The RCC register should be written with interrupts disabled to
        // prevent race conditions with other threads.
        RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
        RCC_SYNC();
    }
    
    // Configure timer
    TIM4->CR1=0;
    TIM4->ARR=0xffff;
    TIM4->CCR1=0;
    TIM4->CCR2=0;
    TIM4->CCR3=0;
    TIM4->CCR4=0;
    // Configure interrupt on timer overflow
    TIM4->DIER=TIM_DIER_UIE;
    NVIC_SetPriority(TIM4_IRQn,13); //Low priority for timer IRQ
    NVIC_EnableIRQ(TIM4_IRQn);
    // Set default parameters
    setFrequency(50);
    setMinPulseWidth(1000);
    setMaxPulseWidth(2000);
}

void SynchronizedServo::precomputeCoefficients()
{
    float realPeriod=1.0f/getFrequency();
    a=65536.0f*(maxWidth-minWidth)/realPeriod;
    b=65536.0f*minWidth/realPeriod;
}

unsigned int SynchronizedServo::getPrescalerInputFrequency()
{
    // The global variable SystemCoreClock from ARM's CMSIS allows to know
    // the CPU frequency.
    unsigned int freq=SystemCoreClock;
    
    //The position of the PPRE1 bit in RCC->CFGR is different in some stm32
    #ifdef _ARCH_CORTEXM3_STM32F1
    const unsigned int ppre1=8;
    #else //stm32f2 and f4
    const unsigned int ppre1=10;
    #endif
    
    // The timer frequency may however be a submultiple of the CPU frequency,
    // due to the bus at whch the periheral is connected being slower. The
    // RCC->CFGR register tells us how slower the APB1 bus is running.
    // This formula takes into account that if the APB1 clock is divided by a
    // factor of two or greater, the timer is clocked at twice the bus
    // interface. After this, the freq variable contains the frequency in Hz
    // at which the timer prescaler is clocked.
    if(RCC->CFGR & RCC_CFGR_PPRE1_2) freq/=1<<((RCC->CFGR>>ppre1) & 0x3);
    
    return freq;
}

void SynchronizedServo::IRQwaitForTimerOverflow(FastInterruptDisableLock& dLock)
{
    waiting=Thread::IRQgetCurrentThread();
    do {
        Thread::IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::yield();
        }
    } while(waiting);
}

} //namespace miosix

 
