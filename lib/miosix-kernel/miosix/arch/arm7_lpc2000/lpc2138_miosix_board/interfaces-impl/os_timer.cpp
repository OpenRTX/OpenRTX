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

void TIM0_Routine() __attribute__ ((interrupt("IRQ"),naked));

class LPC2138Timer0 : public TimerAdapter<LPC2138Timer0, 32>
{
public:
    static inline unsigned int IRQgetTimerCounter() { return T0TC; }
    static inline void IRQsetTimerCounter(unsigned int v) { T0TC=v; }

    static inline unsigned int IRQgetTimerMatchReg() { return T0MR0; }
    static inline void IRQsetTimerMatchReg(unsigned int v) { T0MR0=v; }

    static inline bool IRQgetOverflowFlag() { return T0IR & (1<<3); }
    static inline void IRQclearOverflowFlag() { T0IR = (1<<3); }
    
    static inline bool IRQgetMatchFlag() { return T0IR & (1<<0); }
    static inline void IRQclearMatchFlag() { T0IR = (1<<0); }
    
    static inline void IRQforcePendingIrq() { VICSoftInt=(1<<4); }

    static inline void IRQstopTimer() { T0TCR=0; }
    static inline void IRQstartTimer() { T0TCR=1; }
    
    static unsigned int IRQTimerFrequency() { return miosix::TIMER_CLOCK; }
    
    static void IRQinitTimer()
    {
        PCONP|=(1<<1);      //Enable TIMER0
        T0TCR=0;            //Stop timer
        T0CTCR=0;           //Select "timer mode"
        T0TC=0;             //Counter starts from 0
        T0PR=0;             //No prescaler
        T0PC=0;             //Prescaler counter starts from 0
        T0MR0=0;            //Using MR0 as match interrupt
        T0MR3=0;            //Using MR3 as overflow interrupt (overflow @ 0)
        T0MCR=(1<<0)|(1<<9);//Generate interrupt on MR0 & MR3, no reset on match
        T0CCR=0;            //No capture
        T0EMR=0;            //No pin toggle on match
        //Init VIC
        VICSoftIntClr=(1<<4);   //Clear timer interrupt flag (if previously set)
        VICIntSelect&=~(1<<4);  //Timer0=IRQ
        VICIntEnable=(1<<4);    //Timer0 interrupt ON
        VICVectCntl0=0x20 | 0x4;//Slot 0 of VIC used by Timer0
        VICVectAddr0=reinterpret_cast<unsigned long>(&TIM0_Routine);
    }
};

static LPC2138Timer0 timer;
DEFAULT_OS_TIMER_INTERFACE_IMPLMENTATION(timer);

void TIM0_Routine()
{
    saveContextFromIrq();
    asm volatile("bl _ZN6miosix11osTimerImplEv");
    restoreContext();
}

void __attribute__((noinline)) osTimerImpl()
{
    VICSoftIntClr=(1<<4);//Cleared before as IRQhandler() may set it again
    timer.IRQhandler();
    VICVectAddr=0xff;//Restart VIC
}

} //namespace miosix
