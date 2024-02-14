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

#include "power_manager.h"
#include "board_settings.h"
#include "transceiver.h"
#include "interfaces/bsp.h"
#include <stdexcept>
#include <sys/ioctl.h>
#include "stdio.h"
using namespace std;

namespace miosix {

//
// class PowerManager
//

PowerManager& PowerManager::instance()
{
    static PowerManager singleton;
    return singleton;
}

void PowerManager::deepSleepUntil(long long int when/*, Unit unit*/)
{
    //NOTE: the possibility to sleep in ticks (of the RTC) has been removed when
    //the interface was changed from uncorrected ns to corrected ns.
    //if(unit==Unit::NS){
        // This conversion is very critical: we can't use the straightforward method
        // due to the approximation make during the conversion (necessary to be efficient).
        // ns-->tick(HF)->
        //                  represent a different time. 
        //                  This difference grows with the increasing of time value.
        // ns-->tick(LF)->/
        //
        // This brings problem if we need to do operation that involves the use 
        // of both clocks, aka the sleep (or equivalent) and the deep sleep
        // In this way, I do the
        // ns->tick(HR)[approximated]->tick(LF)[equivalent to the approx tick(HR)]
        // This operation is of course slower than usual, but affordable given 
        // the fact that the deep sleep should be called quite with large time span.
        
        unsigned long long temp=vt.corrected2uncorrected(tc.ns2tick(when));
        //unsigned long long temp=tc.ns2tick(when);
        when=temp*32/46875;
    //}
    
    Transceiver& rtx=Transceiver::instance();
    
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);
    
    Lock<FastMutex> l(powerMutex);  //To access reference counts freely
    PauseKernelLock pkLock;         //To run unexpected IRQs without context switch
    FastInterruptDisableLock dLock; //To do everything else atomically

    const int timeToSyncAfterWakeup = 3;
    
    //The wakeup time has been profiled, and takes ~310us when the transceiver
    //has to be enabled, and ~100us with the transceiver disabled.
    //so we wake up that time before, plus some margin:
    //transceiver enabled:  12 ticks 366us (56us margin)
    //transceiver disabled:  5 ticks 152us (52us margin)
    const int wakeupTime = timeToSyncAfterWakeup + (transceiverPowerDomainRefCount>0 ? 12 : 5);
    
    //Avoid the capture of not significative trigger
    TIMER2->CC[2].CTRL=0;
    
    long long preWake=when-wakeupTime;
    //EFM32 compare channels trigger 1 tick late (undocumented quirk)
    RTC->COMP1=(preWake-1) & 0xffffff;
    while(RTC->SYNCBUSY & RTC_SYNCBUSY_COMP1) ;
    RTC->IFC=RTC_IFC_COMP1;
    RTC->IEN |= RTC_IEN_COMP1;
    //NOTE: the corner case where the wakeup is now is considered "in the past"
    if(preWake<=rtc.IRQgetValue())
    {
        RTC->IFC=RTC_IFC_COMP1;
        RTC->IEN &= ~RTC_IEN_COMP1;
    } else {
        IRQpreDeepSleep(rtx);
        // Flag to enable the deepsleep when we will call _WFI, 
        // otherwise _WFI is translated as a simple sleep status, this means that the core is not running 
        // but all the peripheral (HF and LF), are still working and they can trigger exception
        SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; 
        EMU->CTRL=0;
        for(;;)
        {
            //First, try to go to sleep. If an interrupt occurred since when
            //we have disabled them, this is executed as a NOP
            IRQmakeSureTransceiverPowerDomainIsDisabled();
            __WFI();
            IRQrestartHFXOandTransceiverPowerDomainEnable();
            //If the interrupt we want is now pending, everything is ok
            if(NVIC_GetPendingIRQ(RTC_IRQn))
            {
                //Clear the interrupt (both in the RTC peripheral and NVIC),
                //this is important as pending IRQ prevent WFI from working
                
                if((preWake-1)<=rtc.IRQgetValue()){
                    RTC->IFC=RTC_IFC_COMP1;
                    NVIC_ClearPendingIRQ(RTC_IRQn);
                    break;
                }else{
                    FastInterruptEnableLock eLock(dLock);
                    // Here interrupts are enabled, so the software part of RTC 
                    // can be updated
                    // NOP operation to be sure that the interrupt can be executed
                    __NOP();
                }
                NVIC_ClearPendingIRQ(RTC_IRQn);
            } else {
                //Else we are in an uncomfortable situation: we're waiting for
                //a specific interrupt, but we didn't go to sleep as another
                //interrupt occurred. The core won't allow the WFI to put us to
                //sleep till we serve the interrupt, so let's do it.
                //Note that since the kernel is paused the interrupt we're
                //serving can't cause a context switch and fuck up things.
                IRQresyncClock();
                {
                    FastInterruptEnableLock eLock(dLock);
                    //Here interrupts are enabled, so the interrupt gets served
                    __NOP();
                }
            }
            if(preWake-1<=rtc.IRQgetValue()){
                break;
            }
        }
        // Simple sleep when you call _WFI, for example in the Idle thread
        SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk; 
        //We need this interrupt to continue the sync of HRT with RTC
        RTC->IEN &= ~RTC_IEN_COMP1; 
        IRQpostDeepSleep(rtx);
    }
    //Post deep sleep wait to absorb wakeup time jitter, jitter due to physical phenomena like XO stabilization
    IRQresyncClock();
        
    //EFM32 compare channels trigger 1 tick late (undocumented quirk)
    RTC->COMP1=(when-1) & 0xffffff;
    while(RTC->SYNCBUSY & RTC_SYNCBUSY_COMP1) ;
    RTC->IFC=RTC_IFC_COMP1;
    RTC->IEN |= RTC_IEN_COMP1;
    //I have to use polling because the interrupt are still disabled
    //This "polling" is particular: __WFI make sleeping the core, only if there aren't pending IRQ. 
    //When a generic interrupt triggers, 
    //the core wakes and check the condition. 
    //If true, it goes forward, otherwise the interrupt is caused by another IRQ. 
    //But this IRQ can't be served because the interrupts are disabled, hence the while-cycle turns in a polling-cycle 
    //(bad and not low power, but definitely very rare)
    while(when>rtc.IRQgetValue()) __WFI();
    RTC->IEN &= ~RTC_IEN_COMP1;
    RTC->IFC=RTC_IFC_COMP1;

    //Clean flag to not resync immediately
    TIMER2->IFC=TIMER_IFC_CC2;
    TIMER2->CC[2].CCV;
    
    RTC->COMP1=HRTB::nextSyncPointRtc-1;
}

void PowerManager::enableTransceiverPowerDomain()
{   
    Lock<FastMutex> l(powerMutex);
    if(transceiverPowerDomainRefCount==0)
    {
        //Enable power domain
        transceiver::vregEn::high();
        
        transceiver::reset::low();
        
        //The voltage at the the flash rises in about 70us,
        //observed using an oscilloscope. The voltage at the
        //transceiver is even faster, but the spec says 100us
        //TODO: power hungry, but we can't use the rtc as only one thread
        //can use it at a time, and we may be called by another thread
        delayUs(100);
        
        transceiver::cs::high();

        flash::cs::high();
        flash::hold::high();
        
        spi.enable();
    }
    transceiverPowerDomainRefCount++;
}

void PowerManager::disableTransceiverPowerDomain()
{
    Lock<FastMutex> l(powerMutex);
    transceiverPowerDomainRefCount--;
    if(transceiverPowerDomainRefCount==0)
    {
        //Disable power domain
        spi.disable();
        
        flash::hold::low();
        flash::cs::low();
        
        transceiver::reset::low();
        transceiver::cs::low();
        
        transceiver::vregEn::low();
    } else if(transceiverPowerDomainRefCount<0) {
        throw runtime_error("transceiver power domain refcount is negative");
    }
}

bool PowerManager::isTransceiverPowerDomainEnabled() const
{
    return transceiverPowerDomainRefCount>0;
}

void PowerManager::enableSensorPowerDomain()
{
    #if WANDSTEM_HW_REV>13
    Lock<FastMutex> l(powerMutex);
    if(sensorPowerDomainRefCount==0)
    {
        powerSwitch::high();
    }
    sensorPowerDomainRefCount++;
    #else
    //Nodes prior to rev 1.4 have no separate sensor power domain
    enableTransceiverPowerDomain();
    #endif
}

void PowerManager::disableSensorPowerDomain()
{
    #if WANDSTEM_HW_REV>13
    Lock<FastMutex> l(powerMutex);
    sensorPowerDomainRefCount--;
    if(sensorPowerDomainRefCount==0)
    {
        powerSwitch::low();
    } else if(sensorPowerDomainRefCount<0) {
        throw runtime_error("sensor power domain refcount is negative");
    }
    #else
    //Nodes prior to rev 1.4 have no separate sensor power domain
    disableTransceiverPowerDomain();
    #endif
}

bool PowerManager::isSensorPowerDomainEnabled() const
{
    #if WANDSTEM_HW_REV>13
    return sensorPowerDomainRefCount>0;
    #else
    //Nodes prior to rev 1.4 have no separate sensor power domain
    return isTransceiverPowerDomainEnabled();
    #endif
}

void PowerManager::enableHighRegulatorVoltage()
{
    //Nodes prior to rev 1.3 have no switching voltage regulator
    #if WANDSTEM_HW_REV>12
    Lock<FastMutex> l(powerMutex);
    if(regulatorVoltageRefCount==0)
    {
        voltageSelect::high();
        
        //The voltage regulator, under no load, takes about 500us to increase
        //the voltage, observed using an oscilloscope. Using 2ms to be on the
        //safe side if the regulator is heavily loaded. TODO: characterize it
        Thread::sleep(2);
    }
    regulatorVoltageRefCount++;
    #endif
}

void PowerManager::disableHighRegulatorVoltage()
{
    //Nodes prior to rev 1.3 have no switching voltage regulator
    #if WANDSTEM_HW_REV>12
    Lock<FastMutex> l(powerMutex);
    regulatorVoltageRefCount--;
    if(regulatorVoltageRefCount==0)
    {
        voltageSelect::low();
    } else if(regulatorVoltageRefCount<0) {
        throw runtime_error("voltage regulator refcount is negative");
    }
    #endif
}

bool PowerManager::isRegulatorVoltageHigh()
{
    #if WANDSTEM_HW_REV>12
    return regulatorVoltageRefCount>0;
    #else
    //Nodes prior to rev 1.3 have no switching voltage regulator
    return true;
    #endif
}

PowerManager::PowerManager()
    : transceiverPowerDomainRefCount(0),
      sensorPowerDomainRefCount(0),
      regulatorVoltageRefCount(0),
      spi(Spi::instance()),
      b(HRTB::instance()),
      rtc(Rtc::instance()),
      vht(VHT::instance()),
      vt(VirtualClock::instance()),
      tc(EFM32_HFXO_FREQ) {}

void PowerManager::IRQpreDeepSleep(Transceiver& rtx)
{
    if(transceiverPowerDomainRefCount>0)
    {
        wasTransceiverTurnedOn=rtx.IRQisTurnedOn();
        
        //Disable power domain (this also disables the transceiver)
        spi.disable();
        flash::hold::low();
        flash::cs::low();
        transceiver::reset::low();
        transceiver::cs::low();
    }
    
    #if WANDSTEM_HW_REV>13
    if(sensorPowerDomainRefCount>0) powerSwitch::low();
    #endif
    
    //Not setting the voltage regulator to low voltage for now
}

void PowerManager::IRQmakeSureTransceiverPowerDomainIsDisabled()
{
    //This is called right before going deep sleep, unconditionally disable
    //voltage regulator power domain
    transceiver::vregEn::low();
}

void PowerManager::IRQrestartHFXOandTransceiverPowerDomainEnable()
{
    //This function implements an optimization to improve the time to restore
    //the system state when exiting deep sleep: we need to restart the MCU
    //crystal oscillator, and this takes around 100us. Then, we also need to
    //restart the cc2520 power domain and wait for the voltage to be stable,
    //which incidentally requires again 100us. So, we do both things in
    //parallel.
    //However, this introduces two complications:
    //- additional wakeups: we enabling the power domain but then find out it's
    //  not the right wakeup time. So we need to disable it again when going
    //  back to sleep. IRQmakeSureTransceiverPowerDomainIsDisabled() does this
    //- if the __WFI() is turned into a nop, the HFXO may remain enabled, thus
    //  we won't have our delay. We use transceiverPowerDomainExplicitDelayNeeded
    //  to store whether an explict delay is needed
    
    if(transceiverPowerDomainRefCount>0) transceiver::vregEn::high();
    transceiverPowerDomainExplicitDelayNeeded=
        CMU->STATUS & CMU_STATUS_HFXOENS ? true : false;
    
    CMU->OSCENCMD=CMU_OSCENCMD_HFXOEN; 
    CMU->CMD=CMU_CMD_HFCLKSEL_HFXO;	//This locks the CPU till clock is stable
					//because the HFXO won't emit any waveform until it is stabilized
    CMU->OSCENCMD=CMU_OSCENCMD_HFRCODIS; //turn off RC oscillator
}

void PowerManager::IRQpostDeepSleep(Transceiver& rtx)
{
    if(transceiverPowerDomainRefCount>0)
    {
        if(transceiverPowerDomainExplicitDelayNeeded) delayUs(100);

        flash::cs::high();
        flash::hold::high();
        spi.enable();
        
        if(wasTransceiverTurnedOn)
        {
            transceiver::reset::high();

            //wait until SO=1 (clock stable and running)
            //The simplest possible implementation is
            //while(internalSpi::miso::value()==0) ;
            //but it is too energy hungry
            //internalSpi::miso is PD1, so 1<<1
            GPIO->IFC=1<<1;
            GPIO->IEN |= (1<<1);
            while(internalSpi::miso::value()==0) __WFI();
            GPIO->IFC=1<<1;
            GPIO->IEN &= ~(1<<1);
            transceiver::cs::high();
            
            rtx.configure();
        }
    }
    
    #if WANDSTEM_HW_REV>13
    if(sensorPowerDomainRefCount>0) powerSwitch::high();
    #endif
}

void PowerManager::IRQresyncClock(){
    long long nowRtc=rtc.IRQgetValue();
    long long syncAtRtc=nowRtc+2;
    //This is very important, we need to restore the previous value in COMP1, to gaurentee the proper wakeup
    long long prevCOMP1=RTC->COMP1;
    RTC->COMP1=syncAtRtc-1;
    //Virtual high resolution timer, init without starting the input mode!
    TIMER2->CC[2].CTRL=TIMER_CC_CTRL_ICEDGE_RISING
			| TIMER_CC_CTRL_FILT_DISABLE
			| TIMER_CC_CTRL_INSEL_PRS
			| TIMER_CC_CTRL_PRSSEL_PRSCH4
			| TIMER_CC_CTRL_MODE_INPUTCAPTURE;
    PRS->CH[4].CTRL= PRS_CH_CTRL_SOURCESEL_RTC | PRS_CH_CTRL_SIGSEL_RTCCOMP1;
    RTC->IFC=RTC_IFC_COMP1;
    
    //Clean the buffer to avoid false reads
    TIMER2->CC[2].CCV;
    TIMER2->CC[2].CCV;
    
    while(RTC->SYNCBUSY & RTC_SYNCBUSY_COMP1);

    while(!(RTC->IF & RTC_IF_COMP1));
    long long timestamp=b.IRQgetVhtTimestamp();
    //Got the values, now polishment of flags and register
    RTC->IFC=RTC_IFC_COMP1;
    TIMER2->IFC=TIMER_IFC_CC2;
    
    RTC->COMP1=prevCOMP1;
    while(RTC->SYNCBUSY & RTC_SYNCBUSY_COMP1);
    long long syncAtHrt=mul64x32d32(syncAtRtc, 1464, 3623878656);
    HRTB::clockCorrection=syncAtHrt-timestamp;
    HRTB::syncPointHrtExpected=syncAtHrt;
    HRTB::nextSyncPointRtc=syncAtRtc+HRTB::syncPeriodRtc;
    HRTB::syncPointHrtTheoretical=syncAtHrt;
    HRTB::syncPointHrtActual=syncAtHrt;
    vht.IRQoffsetUpdate(HRTB::syncPointHrtTheoretical,HRTB::syncPointHrtExpected);
}

} //namespace miosix
