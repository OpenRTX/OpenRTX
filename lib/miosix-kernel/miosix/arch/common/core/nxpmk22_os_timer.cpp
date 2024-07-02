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

/*
 * This file implements the functions for the IRQ timer on the NXP MK22F51212 series of MCU.
 * The timer used is the FlexTimer 0 (FTM0). This timer is clocked by the Bus Clock. 
 * The timer is a 16 bits timer. A prescaler of 4 was selected for a clocking frequency
 * of 59.904 MHz / 4 = 14.976 MHz. This makes it possible to have an integer number of clock
 * cycles in 1ms. This also provokes a timer overflow every 65536/14976000 = 4.38 ms.
 * 
 * In order to keep an integer number of cycles in a millisecond, the prescaler can go up to
 * the maximum value but at that point, the resolution of the timer will of course degrade.
 */

#include "kernel/kernel.h"
#include "interfaces/os_timer.h"
#include "interfaces/arch_registers.h"

namespace miosix {

class MK22FlexTimer0 : public TimerAdapter<MK22FlexTimer0, 16>
{
public:
    static inline unsigned int IRQgetTimerCounter() { return FTM0->CNT; }
    static inline void IRQsetTimerCounter(unsigned int v)
    {
        FTM0->CNTIN = v;
        __NOP();
        FTM0->CNT = v; // When CNT is written to, it gets updated with CNTIN value
        FTM0->CNTIN = 0;
    }

    static inline unsigned int IRQgetTimerMatchReg() { return FTM0->CONTROLS[0].CnV; }
    static inline void IRQsetTimerMatchReg(unsigned int v) { FTM0->CONTROLS[0].CnV=v; }

    static inline bool IRQgetOverflowFlag() { return FTM0->SC & FTM_SC_TOF_MASK; }
    static inline void IRQclearOverflowFlag() { FTM0->SC &= ~FTM_SC_TOF(1); } // This will only work if SC was read while TOF was set and setting the bit to 0.
    
    static inline bool IRQgetMatchFlag() { return FTM0->CONTROLS[0].CnSC & FTM_CnSC_CHF(1); }
    static inline void IRQclearMatchFlag() { FTM0->CONTROLS[0].CnSC &= ~FTM_CnSC_CHF(1); }
    
    static inline void IRQforcePendingIrq() { NVIC_SetPendingIRQ(FTM0_IRQn); }

    static inline void IRQstopTimer() { FTM0->SC &= ~FTM_SC_CLKS(3); }
    static inline void IRQstartTimer() { FTM0->SC |= FTM_SC_CLKS(1); }
    
    static unsigned int IRQTimerFrequency()
    {
        /* The global variable SystemCoreClock from ARM's CMSIS allows to know
         * the CPU frequency. From there we get the common clock source to both system clock and 
         * bus clock. Then divide by the clock divider for the bus clock. 
         * Finally we take the prescaler into account.
         */
        
        uint32_t mcgoutClock = SystemCoreClock * (((SIM->CLKDIV1 & SIM_CLKDIV1_OUTDIV1_MASK) >> SIM_CLKDIV1_OUTDIV1_SHIFT) + 1);
        uint32_t busClock = mcgoutClock / (((SIM->CLKDIV1 & SIM_CLKDIV1_OUTDIV2_MASK) >> SIM_CLKDIV1_OUTDIV2_SHIFT) + 1);
        uint32_t freq = busClock >> (FTM0->SC & FTM_SC_PS_MASK);
        return freq;
    }
    
    static void IRQinitTimer()
    {
        // Enable clock to FTM0
        SIM->SCGC6 |= SIM_SCGC6_FTM0(1);

        // Set counter to count up to maximum value, counter initializes at 0
        FTM0->CNTIN = 0x0000;
        FTM0->MOD = 0xFFFF;

        // Set output compare mode without gpio output but with
        // interrupt enabled
        FTM0->CONTROLS[0].CnSC = FTM_CnSC_CHF(0) | FTM_CnSC_CHIE(1) |
                                 FTM_CnSC_MSB(0) | FTM_CnSC_MSA(1) | 
                                 FTM_CnSC_ELSB(0) | FTM_CnSC_ELSA(0) |
                                 FTM_CnSC_ICRST(0) | FTM_CnSC_DMA(0);
        
        // Enable interrupts, keep timer disabled, set prescaler to 4
        FTM0->SC = FTM_SC_TOF(0) | FTM_SC_TOIE(1) | FTM_SC_CPWMS(0) |
                   FTM_SC_CLKS(0) | FTM_SC_PS(2);

        // Enable interrupts for FTM0
        NVIC_SetPriority(FTM0_IRQn, 3); //High priority for FTM0 (Max=0, min=15)
        NVIC_EnableIRQ(FTM0_IRQn);
     
        // FTMEN set to 0, when writing to registers, their value 
        // is updated:
        //  - next system clock cycle for CNTIN
        //  - when FTM counter changes from MOD to CNTIN for MOD
        //  - when the FTM counter updates for CnV
        FTM0->MODE = FTM_MODE_FAULTIE(0) | FTM_MODE_FAULTM(0) |
                     FTM_MODE_CAPTEST(0) | FTM_MODE_PWMSYNC(0) |
                     FTM_MODE_WPDIS(0) | FTM_MODE_INIT(0) |
                     FTM_MODE_FTMEN(0);
    }
};

static MK22FlexTimer0 timer;
DEFAULT_OS_TIMER_INTERFACE_IMPLMENTATION(timer);
} //namespace miosix

void __attribute__((naked)) FTM0_IRQHandler()
{
    saveContext();
    asm volatile ("bl _Z11osTimerImplv");
    restoreContext();
}

void __attribute__((used)) osTimerImpl()
{
    miosix::timer.IRQhandler();
}
