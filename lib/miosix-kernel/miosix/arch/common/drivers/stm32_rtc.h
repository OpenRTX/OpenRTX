/***************************************************************************
 *   Copyright (C) 2017 by Terraneo Federico                               *
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

#include <kernel/timeconversion.h>

namespace miosix {

///
/// Uncomment this directive to enable the RTC clock output functionality
///

//#define RTC_CLKOUT_ENABLE

/**
 * Puts the MCU in deep sleep until the specified absolute time.
 * \param value absolute wait time in nanoseconds
 * If value of absolute time is in the past no waiting will be set
 * and function return immediately.
 */
void absoluteDeepSleep(long long value);

/**
 * Driver for the stm32 RTC.
 * All the wait and deepSleep functions cannot be called concurrently by
 * multiple threads.
 */
class Rtc
{
public:
    /**
     * \return an instance of this class
     */
    static Rtc& instance();

    /**
     * \return the timer counter value in nanoseconds
     */
    long long getValue() const;

    /**
     * \return the timer counter value in nanoseconds
     * 
     * Can be called with interrupt disabled, or inside an interrupt
     */
    long long IRQgetValue() const;

    /**
     * Set the timer counter value
     * \param value new timer value in nanoseconds
     * 
     * NOTE: if alarm is set wakeup time is not updated
     */
//     void setValue(long long value);
    
    /**
     * Put thread in wait for the specified relative time.
     * This function wait for a relative time passed as parameter.
     * \param value relative time to wait, expressed in nanoseconds
     */
    void wait(long long value);
    
    /**
     * Puts the thread in wait until the specified absolute time.
     * \param value absolute wait time in nanoseconds
     * If value of absolute time is in the past no waiting will be set
     * and function return immediately.
     * \return true if the wait time was in the past
     */
    bool absoluteWait(long long value);

    /**
     * \return the timer frequency in Hz
     */
    unsigned int getTickFrequency() const { return 16384; }

private:
    Rtc();
    Rtc(const Rtc&)=delete;
    Rtc& operator= (const Rtc&)=delete;
    
    TimeConversion tc;
    
    friend void absoluteDeepSleep(long long value);
};

} //namespace miosix

#endif //RTC_H
