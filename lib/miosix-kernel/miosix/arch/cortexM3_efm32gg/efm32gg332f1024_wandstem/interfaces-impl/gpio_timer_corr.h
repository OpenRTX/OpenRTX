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

#ifndef GPIO_TIMER_CORR_H
#define GPIO_TIMER_CORR_H

#include "timer_interface.h"
#include "hrtb.h"

namespace miosix{
class GPIOtimerCorr : public HardwareTimer{
public:
        long long getValue() const;
        
        void wait(long long value);
        bool absoluteWait(long long value);
        
        static GPIOtimerCorr& instance();
        bool waitTimeoutOrEvent(long long value);
        bool absoluteWaitTimeoutOrEvent(long long value);
        
        /**
        * Set the timer interrupt to occur at an absolute value and put the 
        * thread in wait of this. 
        * When the timer interrupt will occur, the associated GPIO passes 
        * from a low logic level to a high logic level for few us.
        * \param value absolute value when the interrupt will occur, expressed in 
        * number of tick of the count rate of timer.
        * If value of absolute time is in the past no waiting will be set
        * and function return immediately. In this case, the GPIO will not be
        * pulsed
        * \return true if the wait time was in the past, in this case the GPIO
        * has not been pulsed
        */ 
        bool absoluteWaitTrigger(long long tick);
        
        inline unsigned int getTickFrequency() const{ return b.getTimerFrequency(); }
        
        long long getExtEventTimestamp(Correct c) const;
        
        long long tick2ns(long long tick);
        long long ns2tick(long long ns);
        
    private:
        GPIOtimerCorr();
        GPIOtimerCorr(const GPIOtimerCorr&)=delete;
        GPIOtimerCorr& operator= (const GPIOtimerCorr&)=delete;
        HRTB& b;
        bool isInput; 
        TimeConversion tc;
        static const int stabilizingTime;

};
}
#endif /* GPIO_TIMER_CORR_H */

