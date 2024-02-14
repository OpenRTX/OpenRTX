/***************************************************************************
 *   Copyright (C) 2016 by Fabiano Riccardi                                *
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

#include "vht.h"
#include "virtual_clock.h"
#include "gpio_timer_corr.h"

/**
 * Implementare tutte le correzioni come il trasceiver! Attenzione a bloccare 
 * l'utente se chiede tempi di attesa molto lunghi the il trigger e il timeout!!
 */

namespace miosix{

static VHT* vht=nullptr;
static VirtualClock *vt=nullptr;

/**
 * NOTE: this is a simple forward class to HighResolutionTimerBase.
 * The reason is mainly efficiency of execution, 
 * keeping the function in the same file permits inlining.
 */

long long GPIOtimerCorr::getValue() const{
    FastInterruptDisableLock dLock;
    return vt->uncorrected2corrected(b.IRQgetCurrentTickVht());
}

void GPIOtimerCorr::wait(long long tick){
    Thread::nanoSleep(tc.tick2ns(tick));
}

bool GPIOtimerCorr::absoluteWait(long long tick){
    if(b.getCurrentTick()>=tick){
        return true;
    }
    Thread::nanoSleepUntil(tc.tick2ns(tick));
    return false;
}

//NOTE: Think about how to set the right ms32chkp related to the captured timestamp
long long GPIOtimerCorr::getExtEventTimestamp(Correct c) const{
    long long t=(vht->uncorrected2corrected(b.addBasicCorrection(b.IRQgetSetTimeGPIO()-stabilizingTime)));
    if(c==Correct::UNCORR){
        return t;
    }else{
        return vt->uncorrected2corrected(t);
    }
}

bool GPIOtimerCorr::absoluteWaitTimeoutOrEvent(long long tick){
    return b.gpioAbsoluteWaitTimeoutOrEvent(b.removeBasicCorrection(vht->corrected2uncorrected(vt->corrected2uncorrected(tick))));
}

bool GPIOtimerCorr::waitTimeoutOrEvent(long long tick){
    //FIXME: why the IRQ version of getCurrentTickVht()?
    return absoluteWaitTimeoutOrEvent(vt->uncorrected2corrected(b.IRQgetCurrentTickVht())+tick);
}

bool GPIOtimerCorr::absoluteWaitTrigger(long long tick){
    if(tick-getValue()>HRTB::syncPeriodHrt*2){
        Thread::nanoSleepUntil(tc.tick2ns(tick-HRTB::syncPeriodHrt));
    }
    // Uncomment this lines together with the one in the main
    // to avoid the jitter
    //vht->stopResyncSoft();
    return b.gpioAbsoluteWaitTrigger(b.removeBasicCorrection(vht->corrected2uncorrected(vt->corrected2uncorrected(tick))));
}

long long GPIOtimerCorr::tick2ns(long long tick){
    return tc.tick2ns(tick);
}

long long GPIOtimerCorr::ns2tick(long long ns){
    return tc.ns2tick(ns);
}

GPIOtimerCorr& GPIOtimerCorr::instance(){
    static GPIOtimerCorr instance;
    return instance;
}

GPIOtimerCorr::GPIOtimerCorr(): b(HRTB::instance()),tc(b.getTimerFrequency()) {
    b.initGPIO();
    vht=&VHT::instance();
    vt=&VirtualClock::instance();
}

/// This parameter was obtained by connecting an output compare to an input
/// capture channel and computing the difference between the expected and
/// captured value. 
///
/// It is believed that it is caused by the internal flip-flop
/// in the input capture stage for resynchronizing the asynchronous input and
/// prevent metastability. The test has also been done on multiple boards.
/// The only open issue is whether this delay of 3 ticks is all at the input
/// capture stage or some of those ticks are in the output compare.
const int GPIOtimerCorr::stabilizingTime = 3;
}

