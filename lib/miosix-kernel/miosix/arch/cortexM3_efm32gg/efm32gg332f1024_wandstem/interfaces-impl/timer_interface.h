/***************************************************************************
 *   Copyright (C) 2016 by Federico Terraneo, Fabiano Riccardi             *
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

#ifndef TIMER_INTERFACE_H
#define TIMER_INTERFACE_H

#include <kernel/timeconversion.h>

namespace miosix {

/**
 * Pure virtual class defining the HardwareTimer interface.
 * This interface includes support for an input capture module, used by the
 * Transceiver driver to timestamp packet reception, and an output compare
 * module, used again by the Transceiver to send packets deterministically.
 */
class HardwareTimer
{
public:
    enum Correct{
        CORR,
        UNCORR
    };
    
    /**
     * \return the timer counter value in ticks
     */
    virtual long long getValue() const=0;

    /**
     * Put thread in wait for the specified relative time.
     * This function wait for a relative time passed as parameter.
     * \param value relative time to wait, expressed in ticks
     */
    virtual void wait(long long value)=0;

    /**
     * Puts the thread in wait for the specified absolute time.
     * \param value absolute wait time in ticks
     * If value of absolute time is in the past no waiting will be set
     * and function return immediately.
     * \return true if the wait time was in the past
     */
    virtual bool absoluteWait(long long value)=0;

    /**
     * Set the timer interrupt to occur at an absolute value and put the 
     * thread in wait of this. 
     * When the timer interrupt will occur, the associated GPIO passes 
     * from a low logic level to a high logic level for few us.
     * \param value absolute value when the interrupt will occur, expressed in 
     * ticks
     * If value of absolute time is in the past no waiting will be set
     * and function return immediately. In this case, the GPIO will not be
     * pulsed
     * \return true if the wait time was in the past, in this case the GPIO
     * has not been pulsed
     */
    virtual bool absoluteWaitTrigger(long long value)=0;

    /**
     * Put thread in waiting of timeout or extern event.
     * \param value timeout expressed in ticks
     * \return true in case of timeout
     */
    virtual bool waitTimeoutOrEvent(long long value)=0;

    /**
     * Put thread in waiting of timeout or extern event.
     * \param value absolute timeout expressed in ticks
     * If value of absolute time is in the past no waiting will be set
     * and function return immediately. If the event has already occurred,
     * return immediately.
     * \return true in case of timeout, or if the wait time is in the past.
     * In the corner case where both the timeout and the event are in the past,
     * return false.
     */
    virtual bool absoluteWaitTimeoutOrEvent(long long value)=0;

    /**
     * \return the precise time in ticks when the IRQ signal of the event was
     * asserted
     */
    virtual long long getExtEventTimestamp(Correct c) const=0;

    /**
     * Althought the interface to the timer is in ticks to be able to do
     * computations that are exact and use the timer resolution fully,
     * these member functions are provided to convert to nanoseconds
     * 
     * \param tick time point in timer ticks
     * \return the equivalent time point in the nanosecond timescale
     */
    virtual long long tick2ns(long long tick)=0;

    /**
     * Althought the interface to the timer is in ticks to be able to do
     * computations that are exact and use the timer resolution fully,
     * these member functions are provided to convert to nanoseconds
     * 
     * \param ns time point in nanoseconds
     * \return the equivalent time point in the timer tick timescale
     */
    virtual long long ns2tick(long long ns)=0;

    /**
     * \return the timer frequency in Hz
     */
    virtual unsigned int getTickFrequency() const=0;
};

/**
 * \return the timer used by the transceiver
 */
HardwareTimer& getTransceiverTimer();

}

#endif /* TIMER_INTERFACE_H */

