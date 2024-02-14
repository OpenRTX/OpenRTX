/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
 *   Copyright (C) 2013, 2014 by Terraneo Federico and Luigi Rinaldi       *
 *   Copyright (C) 2015, 2016 by Terraneo Federico, Luigi Rinaldi and      *
 *   Silvano Seva                                                          *
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

#ifndef RTC_H
#define RTC_H

#include "timer_interface.h"
#include <kernel/timeconversion.h>

namespace miosix {

/**
 * Manages the hardware timer that runs also in low power mode.
 * This class is not safe to be accessed by multiple threads simultaneously.
 */
class Rtc : public HardwareTimer
{
public:
    /**
     * \return a reference to the timer (singleton)
     */
    static Rtc& instance();
    
    /**
     * \return the timer counter value in ticks
     */
    long long getValue() const;
    
    /**
     * \return the timer counter value in ticks
     * 
     * Can be called with interrupt disabled
     */
    long long IRQgetValue() const;

    /**
     * Set the timer counter value
     * \param value new timer value in ticks
     */
    void setValue(long long value);
    
    /**
     * Put thread in wait for the specified relative time.
     * This function wait for a relative time passed as parameter.
     * \param value relative time to wait, expressed in ticks
     */
    void wait(long long value);
    
    /**
     * Puts the thread in wait for the specified absolute time.
     * \param value absolute wait time in ticks
     * If value of absolute time is in the past no waiting will be set
     * and function return immediately.
     * \return true if the wait time was in the past
     */
    bool absoluteWait(long long value);
    
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
    bool absoluteWaitTrigger(long long value);
    
     /**
     * Put thread in waiting of timeout or extern event.
     * \param value timeout expressed in ticks
     * \return true in case of timeout
     */
    bool waitTimeoutOrEvent(long long value);
    
    /**
     * Put thread in waiting of timeout or extern event.
     * \param value absolute timeout expressed in ticks
     * If value of absolute time is in the past no waiting will be set
     * and function return immediately.
     * \return true in case of timeout, or if the wait time is in the past.
     * In the corner case where both the timeout and the event are in the past,
     * return false.
     */
    bool absoluteWaitTimeoutOrEvent(long long value);
    
    /**
     * \return the precise time in ticks when the IRQ signal of the event was
     * asserted
     */
    long long getExtEventTimestamp(Correct c) const;
    
        /**
     * Althought the interface to the timer is in ticks to be able to do
     * computations that are exact and use the timer resolution fully,
     * these member functions are provided to convert to nanoseconds
     * 
     * \param tick time point in timer ticks
     * \return the equivalent time point in the nanosecond timescale
     */
    long long tick2ns(long long tick);

    /**
     * Althought the interface to the timer is in ticks to be able to do
     * computations that are exact and use the timer resolution fully,
     * these member functions are provided to convert to nanoseconds
     * 
     * \param ns time point in nanoseconds
     * \return the equivalent time point in the timer tick timescale
     */
    long long ns2tick(long long ns);
    
    /**
     * \return the timer frequency in Hz
     */
    unsigned int getTickFrequency() const;
    
    /// The internal RTC frequency in Hz
    static const unsigned int frequency=32768;
    
private:
    /**
     * Constructor
     */
    Rtc();
    Rtc(const Rtc&)=delete;
    Rtc& operator=(const Rtc&)=delete;
    
    TimeConversion tc; ///< Class for converting from nanoseconds to ticks
};

} //namespace miosix

#endif //RTC_H
