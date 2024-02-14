/***************************************************************************
 *   Copyright (C) 2021 by Terraneo Federico                               *
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

#include "kernel/kernel.h"
#include "interfaces/os_timer.h"
#include "interfaces/arch_registers.h"

namespace miosix {

class STM32Timer5 : public TimerAdapter<STM32Timer5, 32>
{
public:
    static inline unsigned int IRQgetTimerCounter() { return TIM5->CNT; }
    static inline void IRQsetTimerCounter(unsigned int v) { TIM5->CNT=v; }

    static inline unsigned int IRQgetTimerMatchReg() { return TIM5->CCR1; }
    static inline void IRQsetTimerMatchReg(unsigned int v) { TIM5->CCR1=v; }

    static inline bool IRQgetOverflowFlag() { return TIM5->SR & TIM_SR_UIF; }
    static inline void IRQclearOverflowFlag() { TIM5->SR = ~TIM_SR_UIF; }
    
    static inline bool IRQgetMatchFlag() { return TIM5->SR & TIM_SR_CC1IF; }
    static inline void IRQclearMatchFlag() { TIM5->SR = ~TIM_SR_CC1IF; }
    
    static inline void IRQforcePendingIrq() { NVIC_SetPendingIRQ(TIM5_IRQn); }

    static inline void IRQstopTimer() { TIM5->CR1 &= ~TIM_CR1_CEN; }
    static inline void IRQstartTimer() { TIM5->CR1 |= TIM_CR1_CEN; }
    
    static unsigned int IRQTimerFrequency()
    {
        // The global variable SystemCoreClock from ARM's CMSIS allows to know
        // the CPU frequency.
        unsigned int result=SystemCoreClock;

        // The timer frequency may however be a submultiple of the CPU frequency,
        // due to the bus at whch the periheral is connected being slower. The
        // RCC->CFGR register tells us how slower the APB1 bus is running.
        // This formula takes into account that if the APB1 clock is divided by a
        // factor of two or greater, the timer is clocked at twice the bus
        // interface. After this, the freq variable contains the frequency in Hz
        // at which the timer prescaler is clocked.
        #if defined(_ARCH_CORTEXM7_STM32H7)
        if(RCC->D2CFGR & RCC_D2CFGR_D2PPRE1_2) result/=1<<((RCC->D2CFGR>>4) & 0x3);
        #else
        if(RCC->CFGR & RCC_CFGR_PPRE1_2) result/=1<<((RCC->CFGR>>10) & 0x3);
        #endif
        return result;
    }
    
    static void IRQinitTimer()
    {
        #if defined(_ARCH_CORTEXM7_STM32H7)
        RCC->APB1LENR |= RCC_APB1LENR_TIM5EN;
        RCC_SYNC();
        DBGMCU->APB1LFZ1 |= DBGMCU_APB1LFZ1_DBG_TIM5; //Stop while debugging
        #elif defined(_ARCH_CORTEXM4_STM32L4)
        RCC->APB1ENR1 |= RCC_APB1ENR1_TIM5EN;
        RCC_SYNC();
        DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_TIM5_STOP; //Stop while debugging
        #else
        RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
        RCC_SYNC();
        DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_TIM5_STOP; //Stop while debugging
        #endif
        // Setup TIM5 base configuration
        // Mode: Up-counter
        // Interrupts: counter overflow, Compare/Capture on channel 1
        TIM5->CR1=TIM_CR1_URS;
        TIM5->DIER=TIM_DIER_UIE | TIM_DIER_CC1IE;
        NVIC_SetPriority(TIM5_IRQn,3); //High priority for TIM5 (Max=0, min=15)
        NVIC_EnableIRQ(TIM5_IRQn);
        // Configure channel 1 as:
        // Output channel (CC1S=0)
        // No preload(OC1PE=0), hence TIM5_CCR1 can be written at anytime
        // No effect on the output signal on match (OC1M = 0)
        TIM5->CCMR1 = 0;
        TIM5->CCR1 = 0;
        // TIM5 Operation Frequency Configuration: Max Freq. and longest period
        TIM5->PSC = 0;
        TIM5->ARR = 0xFFFFFFFF;
        TIM5->EGR = TIM_EGR_UG; //To enforce the timer to apply PSC
    }
};

static STM32Timer5 timer;
DEFAULT_OS_TIMER_INTERFACE_IMPLMENTATION(timer);
} //namespace miosix

void __attribute__((naked)) TIM5_IRQHandler()
{
    saveContext();
    asm volatile ("bl _Z11osTimerImplv");
    restoreContext();
}

void __attribute__((used)) osTimerImpl()
{
    miosix::timer.IRQhandler();
}
