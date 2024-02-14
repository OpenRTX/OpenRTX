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

#include "gpio_timer.h"

namespace miosix{

/**
 * NOTE: this is a simple forward class to HighResolutionTimerBase.
 * The reason is mainly efficiency of execution, 
 * keeping the function in the same file permits inlining.
 */

long long GPIOtimer::getValue() const{
    FastInterruptDisableLock dLock;
    return b.IRQgetCurrentTick();
}

void GPIOtimer::wait(long long tick){
    Thread::nanoSleep(tc.tick2ns(tick));
}

bool GPIOtimer::absoluteWait(long long tick){
    if(b.getCurrentTick()>=tick){
	return true;
    }
    Thread::nanoSleepUntil(tc.tick2ns(tick));
    return false;
}

//NOTE: Think about how to set the right ms32chkp related to the captured timestamp
long long GPIOtimer::getExtEventTimestamp(Correct c) const{
    return b.IRQgetSetTimeGPIO() - stabilizingTime;
}

bool GPIOtimer::absoluteWaitTimeoutOrEvent(long long tick){
    return b.gpioAbsoluteWaitTimeoutOrEvent(tick);
}

bool GPIOtimer::waitTimeoutOrEvent(long long tick){
    return absoluteWaitTimeoutOrEvent(b.getCurrentTick()+tick);
}

bool GPIOtimer::absoluteWaitTrigger(long long tick){
    return b.gpioAbsoluteWaitTrigger(tick);
}

long long GPIOtimer::tick2ns(long long tick){
    return tc.tick2ns(tick);
}

long long GPIOtimer::ns2tick(long long ns){
    return tc.ns2tick(ns);
}

GPIOtimer::~GPIOtimer() {}

GPIOtimer& GPIOtimer::instance(){
    static GPIOtimer instance;
    return instance;
}

GPIOtimer::GPIOtimer(): b(HRTB::instance()),tc(b.getTimerFrequency()) {
    b.initGPIO();
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
const int GPIOtimer::stabilizingTime = 3;
}
