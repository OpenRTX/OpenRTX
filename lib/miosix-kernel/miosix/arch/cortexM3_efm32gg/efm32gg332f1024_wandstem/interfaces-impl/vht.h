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

#ifndef VHT_H
#define VHT_H

#include "hrtb.h"
#include "kernel/kernel.h"
#include "hrtb.h"
#include "kernel/timeconversion.h"
#include "gpio_timer.h"
#include "transceiver_timer.h"
#include "rtc.h"
#include "flopsync_vht.h"

namespace miosix{
    
class VHT {
public:
    //To keep a precise track of missing sync. 
    //It is incremented in the TMR2->CC2 routine and reset in the Thread 
    static int pendingVhtSync;
    
    static bool softEnable;
    static bool hardEnable;
    
    static VHT& instance();
    
    /**
     * @return 
     */
    static inline long long corrected2uncorrected(long long tick){
        return baseExpected+fastNegMul((tick-baseTheoretical),inverseFactorI,inverseFactorD);
    }
    
    void start();
    
    /**
     * Correct a given tick value with the windows parameter
     * @param tick
     * @return 
     */
    static inline long long uncorrected2corrected(long long tick){
        return baseTheoretical+fastNegMul(tick-baseExpected,factorI,factorD);
    }
    
    void IRQoffsetUpdate(long long baseTheoretical, long long baseComputed);
    
    void update(long long baseTheoretical, long long baseComputed, long long clockCorrection);
    
    void stopResyncSoft();
    void startResyncSoft();
    
private:
    VHT();
    VHT(const VHT&)=delete;
    VHT& operator=(const VHT&)=delete;
    
    static inline long long fastNegMul(long long a,unsigned int bi, unsigned int bf){
        if(a<0){
            return -mul64x32d32(-a,bi,bf);
        }else{
            return mul64x32d32(a,bi,bf);
        }
    }
    
    static void doRun(void *arg);
    
    void loop();
    
    static long long baseTheoretical;
    static long long baseExpected;
    static long long error;
    
    //Multiplicative factor VHT
    static unsigned int factorI;
    static unsigned int factorD;
    static unsigned int inverseFactorI;
    static unsigned int inverseFactorD;
};

}

#endif /* VHT_H */

