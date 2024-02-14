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

#include "transceiver_timer.h"
#include "vht.h"
#include "virtual_clock.h"

using namespace miosix;

static VHT* vht=nullptr;
static VirtualClock *vt=nullptr;

long long TransceiverTimer::getValue() const{
    FastInterruptDisableLock dLock;
    return vt->uncorrected2corrected(b.IRQgetCurrentTickVht());
}

void TransceiverTimer::wait(long long tick){
    absoluteWait(vt->uncorrected2corrected(b.getCurrentTickVht())+tick);
}

bool TransceiverTimer::absoluteWait(long long tick){
    FastInterruptDisableLock dLock;
    
    long long t=b.removeBasicCorrection(vht->corrected2uncorrected(vt->corrected2uncorrected(tick)));
    b.setModeTransceiverTimer(true);
    if(b.IRQsetTransceiverTimeout(t)==WaitResult::WAITING){
        b.IRQtransceiverWait(t,&dLock);
        return false;
    }
    return true;
}

bool TransceiverTimer::absoluteWaitTrigger(long long tick){
    return b.transceiverAbsoluteWaitTrigger(b.removeBasicCorrection(vht->corrected2uncorrected(vt->corrected2uncorrected(tick))));
}

bool TransceiverTimer::absoluteWaitTimeoutOrEvent(long long tick){
    return b.transceiverAbsoluteWaitTimeoutOrEvent(b.removeBasicCorrection(vht->corrected2uncorrected(vt->corrected2uncorrected(tick))));
}

bool TransceiverTimer::waitTimeoutOrEvent(long long tick){
    //FIXME: why the IRQ version of getCurrentTickVht()?
    return absoluteWaitTimeoutOrEvent(vt->uncorrected2corrected(b.IRQgetCurrentTickVht())+tick);
}

long long TransceiverTimer::tick2ns(long long tick){
    return tc.tick2ns(tick);
}

long long TransceiverTimer::ns2tick(long long ns){
    return tc.ns2tick(ns);
}
            
unsigned int TransceiverTimer::getTickFrequency() const{
    return b.getTimerFrequency();
}
	    
long long TransceiverTimer::getExtEventTimestamp(Correct c) const{
    long long t=(vht->uncorrected2corrected(b.addBasicCorrection(b.IRQgetSetTimeTransceiver()-stabilizingTime)));
    if(c==Correct::UNCORR){
        return t;
    }else{
        return vt->uncorrected2corrected(t);
    }
}
	 
TransceiverTimer::TransceiverTimer():b(HRTB::instance()),tc(b.getTimerFrequency()) {
    b.initTransceiver();
    vht=&VHT::instance();
    vt=&VirtualClock::instance();
}

TransceiverTimer& TransceiverTimer::instance(){
    static TransceiverTimer instance;
    return instance;
}

TransceiverTimer::~TransceiverTimer() {}

/// This parameter was obtained by connecting an output compare to an input
/// capture channel and computing the difference between the expected and
/// captured value. 
///
/// It is believed that it is caused by the internal flip-flop
/// in the input capture stage for resynchronizing the asynchronous input and
/// prevent metastability. The test has also been done on multiple boards.
/// The only open issue is whether this delay of 3 ticks is all at the input
/// capture stage or some of those ticks are in the output compare.
const int TransceiverTimer::stabilizingTime=3;
