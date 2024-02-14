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

/***********************************************************************
 * rtc.h Part of the Miosix Embedded OS.
 * Rtc support for the board that initialize correctly the RTC peripheral,
 * and exposes its functionalities.
 ************************************************************************/

#ifndef RTC_H
#define RTC_H

#include <kernel/timeconversion.h>
#include <kernel/kernel.h>

namespace miosix {

void IRQrtcInit();

/**
 * \brief Class implementing the functionalities 
 *        of the RTC peripherla of the board
 *
 * All the wait and deepSleep functions cannot be called concurrently by
 * multiple threads, so there is a single instance of the class that is share * among all the threads
 */
class Rtc
{
public:
    /**
     * \brief Rtc class implements the singleton design pattern
     *
     * \return the only used instance of this class
     */
    static Rtc& instance();

    const unsigned int stopModeOffsetns = 504000;

    /**
     * \brief Setup the wakeup timer for deep sleep interrupts.
     *
     * Function used to setup the wakeup interrupt for the 
     * RTC and the associated NVIC IRQ and EXTI lines.
     * It also store the correct value for the clock used by RTC
     * and the wakeup clock period
     */
    void setWakeupInterrupt();

    /**
     * \brief Set wakeup timer for wakeup interrupt
     *
     * Set wakeup timer for wakeup interrupt according to the
     * procedure described in the reference manual of the
     * STM32F407VG. It doesn't make the timer start
     */
    void setWakeupTimer(unsigned short int);

    /**
     * \brief Starts the periodic wakeup timer of the RTC 
     */
    void startWakeupTimer();
    /**
     * \brief Stop the periodic wakeup timer of the RTC
     */ 
    void stopWakeupTimer();
    /**
     * \brief Get the subseconds value of RTC 
     * \return the Sub second value of the Time register of the RTC
     *         as milliseconds. The value is converted according to 
     * the current clock used to  synchronize the RTC. 
     */
    /* unsigned short int getSSR(); */
    /* unsigned long long int getDate(); */
    /* unsigned long long int getTime(); */

    long long getWakeupOverhead();
    long long getMinimumDeepSleepPeriod();

    Rtc (const Rtc&) = delete;
    Rtc& operator=(const Rtc&) = delete;
private:
    Rtc();
    unsigned int clock_freq; //! Hz set according to the selected clock
    unsigned int wkp_clock_period; //! How many nanoseconds often the wut counter is decreased
    unsigned short int prescaler_s; //! Needed to know the prescaler factor 
    long long wakeupOverheadNs;

    const long long minimumDeepSleepPeriod = 121000; //! the number of nanoseconds for the smallest deep sleep interval
    TimeConversion wkp_tc;

    long int remaining_wakeups = 0; ///! keep track of remaining wakeups for very long deep sleep intervals
    /**
     * not preemptable function that read SSR value of the RTC Time register
     */
    /* unsigned short int IRQgetSSR(); */
    /* /\** */
    /*  * not preemptable function that compute the time of the RTC Time register */
    /*  *\/ */
    /* unsigned long long int IRQgetTime(); */
    /* /\** */
    /*  * not preemptable function that compute the date of the RTC calendar value */
    /*  *\/ */
    /* unsigned long long int IRQgetDate(); */

    friend bool IRQdeepSleep(long long);
};

} //namespace miosix

#endif //RTC_H
