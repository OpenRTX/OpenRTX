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

#include "config/miosix_settings.h"
#include "kernel/kernel.h"
#include "interfaces/os_timer.h"
#include "interfaces/arch_registers.h"
#include "interfaces/delays.h"
#include "kernel/logging.h"

/*
 * Long story short, the ATsam4l has two kind of timers, the TC and AST and both
 * suck in their unique kind of way.
 * 
 * The TC is the only timer I've ever seen whose timer counter is read-only and
 * when the timer is started can only start from zero. So we need a software
 * workaround to advance the timer to account for deep sleep periods. Also, its
 * interrupt flags are cleared by just reading the status register, which to
 * implement the pending bit trick required to reimplement in software the
 * persistance of interrupt flags that any sane peripheral would do in hardware.
 * 
 * The AST would in theory be cool, as it is an RTC and thus it keeps the time
 * also while in deep sleep and can even wake the CPU up, but the RTC clock
 * domain crossing needs to be handled explicitly in software and requires to
 * poll for entire RTC clock cycles slowing down context switches considerably.
 * Moreover, it was found that likely due a clock domain crossing issue in
 * hardware the overflow flag is not in sync with the timer counter! It is
 * asserted 1.5 clock cycles before the observable timer counter rolls over
 * from 0xffffffff to 0x00000000. Yes, it is asserted in the middle of the time
 * when the counter reads 0xfffffffe! This messes up the pending bit trick and
 * without a workaround would cause a call to getTime() in the wrong moment to
 * return a time 0x100000000 / 16384 = 72 hours in the future! This has been
 * observed to stall the entire OS context switches for 72 hours before the
 * workaound was applied.
 * 
 * I guess Atmel really doesn't know how to design a timer that works....
 */

#ifndef WITH_RTC_AS_OS_TIMER

namespace miosix {

/*
 * Apparently, the TC1->TC_CHANNEL[0].TC_SR bits are cleared by... reading
 * the register. Since for the pending bit trick logic we need these
 * interrupt flags to be less volatile than that, we keep a copy of them
 * here. At least I've tested and clearing this flag outside interrupt context
 * doesn't unset the interrupt form being pending, which is a good thing.
 */
static unsigned int sr=0;

/*
 * The timer counter TC1->TC_CHANNEL[0].TC_CV is read-only! So in order to set
 * the timer value we need to keep it in a software variable.
 */
static unsigned short counterAdd=0;

class ATSAM_TC1_Timer : public TimerAdapter<ATSAM_TC1_Timer, 16>
{
public:
    static inline unsigned int IRQgetTimerCounter()
    {
        return TC1->TC_CHANNEL[0].TC_CV+counterAdd;
    }
    static inline void IRQsetTimerCounter(unsigned int v) { counterAdd=v; }

    static inline unsigned int IRQgetTimerMatchReg()
    {
        return TC1->TC_CHANNEL[0].TC_RC+counterAdd;
    }
    static inline void IRQsetTimerMatchReg(unsigned int v)
    {
        TC1->TC_CHANNEL[0].TC_RC=v-counterAdd;
    }
    
    static inline bool IRQgetOverflowFlag()
    {
        sr |= TC1->TC_CHANNEL[0].TC_SR;
        return sr & TC_SR_COVFS;
    }
    static inline void IRQclearOverflowFlag()
    {
        sr &= ~TC_SR_COVFS;
    }
    
    static inline bool IRQgetMatchFlag()
    {
        sr |= TC1->TC_CHANNEL[0].TC_SR;
        return sr & TC_SR_CPCS;
    }
    static inline void IRQclearMatchFlag()
    {
        sr &= ~TC_SR_CPCS;
    }
    
    static inline void IRQforcePendingIrq() { NVIC_SetPendingIRQ(TC10_IRQn); }

    static inline void IRQstopTimer() { TC1->TC_CHANNEL[0].TC_CCR=TC_CCR_CLKDIS; }
    static inline void IRQstartTimer()
    {
        //NOTE: This will start the timer but also reset it, and this is desired
        TC1->TC_CHANNEL[0].TC_CCR=TC_CCR_CLKEN | TC_CCR_SWTRG;
        //For a short while after starting it, the previous value is visible
        delayUs(1);
    }
    
    /*
     * A 16bit timers overflows too frequently if clocked at the maximum speed,
     * and this is bad both because it nullifies the gains of a tickless kernel
     * and because if interrupts are disabled for an entire timer period the
     * OS loses the knowledge of time. For this reason, we set the timer clock
     * to a lower value using the prescaler.
     */
    static const int timerFrequency=1000000; //1MHz
    
    static unsigned int IRQTimerFrequency()
    {
        return timerFrequency;
    }
    
    static void IRQinitTimer()
    {
        int timerInputFreq = SystemCoreClock / 2;
        
        //Handle the case when the prescribed timer frequency is not achievable.
        //For now, we just enter an infinite loop so if someone selects an
        //impossible frequency it won't go unnoticed during testing
        if(timerInputFreq % timerFrequency)
        {
            IRQbootlog("Frequency error\r\n");
            for(;;) ;
        }
        
        //Configure GCLK8 as prescaler for TC1
        SCIF->SCIF_GCCTRL[8].SCIF_GCCTRL = SCIF_GCCTRL_DIV((timerInputFreq/timerFrequency)-1)
                                         | SCIF_GCCTRL_OSCSEL(getSelectedOscillator())
                                         | SCIF_GCCTRL_DIVEN
                                         | SCIF_GCCTRL_CEN;
        
        auto tempPbamask = PM->PM_PBAMASK | PM_PBAMASK_TC1;
        PM->PM_UNLOCK = PM_UNLOCK_KEY(0xaa) | PM_UNLOCK_ADDR(PM_PBAMASK_OFFSET);
        PM->PM_PBAMASK = tempPbamask; //Enable clock gating to TC1
        
        TC1->TC_CHANNEL[0].TC_CMR = TC_CMR_WAVE | TC_CMR_CAPTURE_TCCLKS(0); //CLOCK=GCLK8
        TC1->TC_CHANNEL[0].TC_IER = TC_IER_CPCS | TC_IER_COVFS;
        
        NVIC_SetPriority(TC10_IRQn,3);
        NVIC_EnableIRQ(TC10_IRQn);
    }
};

static ATSAM_TC1_Timer timer;
DEFAULT_OS_TIMER_INTERFACE_IMPLMENTATION(timer);
} //namespace miosix

void __attribute__((naked)) TC10_Handler()
{
    saveContext();
    asm volatile("bl _Z11osTimerImplv");
    restoreContext();
}

void __attribute__((used)) osTimerImpl()
{
    miosix::timer.IRQhandler();
}

#else //WITH_RTC_AS_OS_TIMER

namespace miosix {

// quirkAdvance = 2. One is needed as setting the match register for a tick
// after the current one does not trigger an interrupt, Another one is due to
// the +1 in IRQgetTimerCounter() to account for the pending bit hardware bug
class ATSAM_AST_Timer : public TimerAdapter<ATSAM_AST_Timer, 32, 2>
{
public:
    static inline unsigned int IRQgetTimerCounter()
    {
        while(AST->AST_SR & AST_SR_BUSY) ;
        // Workaround for hardware bug! The pending bit is asserted earler than
        // the observable counter rollover, actually in the middle of the timer
        // counting 0xfffffffe. Solution: return counter value +1 so 0xffffffff
        // becomes 0x0 which makes it in sync, if reading 0xfffffffe wait till
        // next cycle as we can't predict what the pending bit would be
        for(;;)
        {
            auto result=AST->AST_CV;
            if(result==0xfffffffe) continue;
            return result+1;
        }
    }
    static inline void IRQsetTimerCounter(unsigned int v)
    {
        while(AST->AST_SR & AST_SR_BUSY) ;
        AST->AST_CV=v-1;
    }

    static inline unsigned int IRQgetTimerMatchReg()
    {
        while(AST->AST_SR & AST_SR_BUSY) ;
        return AST->AST_AR0;
    }
    static inline void IRQsetTimerMatchReg(unsigned int v)
    {
        while(AST->AST_SR & AST_SR_BUSY) ;
        AST->AST_AR0=v;
    }

    static inline bool IRQgetOverflowFlag() { return AST->AST_SR & AST_SR_OVF; }
    static inline void IRQclearOverflowFlag()
    {
        while(AST->AST_SR & AST_SR_BUSY) ;
        AST->AST_SCR=AST_SCR_OVF;
    }
    
    static inline bool IRQgetMatchFlag() { return AST->AST_SR & AST_SR_ALARM0; }
    static inline void IRQclearMatchFlag()
    {
        while(AST->AST_SR & AST_SR_BUSY) ;
        AST->AST_SCR=AST_SCR_ALARM0;
    }
    
    static inline void IRQforcePendingIrq() { NVIC_SetPendingIRQ(AST_ALARM_IRQn); }

    static inline void IRQstopTimer()
    {
        while(AST->AST_SR & AST_SR_BUSY) ;
        AST->AST_CR &= ~AST_CR_EN;
    }
    static inline void IRQstartTimer()
    {
        while(AST->AST_SR & AST_SR_BUSY) ;
        AST->AST_CR |= AST_CR_EN;
    }
    
    static unsigned int IRQTimerFrequency() { return 16384; }
    
    static void IRQinitTimer()
    {
        start32kHzOscillator();

        //AST clock gating already enabled at boot
        PDBG|=PDBG_AST; //Stop AST during debugging
        
        //Select 32kHz clock
        AST->AST_CLOCK=AST_CLOCK_CSSEL_32KHZCLK;
        while(AST->AST_SR & AST_SR_CLKBUSY) ;
        AST->AST_CLOCK |= AST_CLOCK_CEN;
        while(AST->AST_SR & AST_SR_CLKBUSY) ;

        //Counter mode, not started, clear only on overflow, fastest prescaler
        AST->AST_CR=AST_CR_PSEL(0);
        
        //Initialize conter and match register
        while(AST->AST_SR & AST_SR_BUSY) ;
        AST->AST_CV=0;
        while(AST->AST_SR & AST_SR_BUSY) ;
        AST->AST_SCR=0xffffffff;

        //Interrupt on overflow or match
        AST->AST_IER=AST_IER_ALARM0 | AST_IER_OVF;
        //Wakeup from deep sleep on overflow or match
        while(AST->AST_SR & AST_SR_BUSY) ;
        AST->AST_WER=AST_WER_ALARM0 | AST_WER_OVF;
        
        //High priority for AST (Max=0, min=15)
        NVIC_SetPriority(AST_ALARM_IRQn,3); 
        NVIC_EnableIRQ(AST_ALARM_IRQn);
        NVIC_SetPriority(AST_OVF_IRQn,3);
        NVIC_EnableIRQ(AST_OVF_IRQn);
    }
};

static ATSAM_AST_Timer timer;
DEFAULT_OS_TIMER_INTERFACE_IMPLMENTATION(timer);

/*
// Test code for checking the presence of the race condition. Call from main.
// Set AST->AST_CV=0xffff0000; in timer init not to wait 72 hours till test end.
void test()
{
    FastInterruptDisableLock dLock;
    long long lastgood=0;
    for(;;)
    {
        auto current=timer.IRQgetTimeTick();
        if(current>0x180000000LL)
        {
            FastInterruptEnableLock eLock(dLock);
            iprintf("Test failed fail=0x%llx lastgood=0x%llx\n",current,lastgood);
        } else if(current==0x100001000LL) IRQerrorLog("Test end\r\n");
        lastgood=current;
    }
}*/
} //namespace miosix

void __attribute__((naked)) AST_ALARM_Handler()
{
    saveContext();
    asm volatile("bl _Z11osTimerImplv");
    restoreContext();
}

void __attribute__((naked)) AST_OVF_Handler()
{
    saveContext();
    asm volatile("bl _Z11osTimerImplv");
    restoreContext();
}

void __attribute__((used)) osTimerImpl()
{
    miosix::timer.IRQhandler();
}

#endif //WITH_RTC_AS_OS_TIMER
