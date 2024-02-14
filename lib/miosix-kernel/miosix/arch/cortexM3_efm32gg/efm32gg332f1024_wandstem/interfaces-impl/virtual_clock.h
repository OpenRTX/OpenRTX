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

#ifndef VIRTUAL_CLOCK_H
#define VIRTUAL_CLOCK_H

#include "kernel/timeconversion.h"
#include "cassert"
#include "stdio.h"
#include "miosix.h"

namespace miosix {
/**
 * This class implements an instrument for correcting the clock based on
 * the FLOPSYNC-2 synchronization algorithm. It receives FLOPSYNC-2 data
 * and parameters as input, preserving a state capable of correcting an arbitrary
 * timestamp, using an equation of type texpected = tstart + coeff * skew
 */
class VirtualClock {
public:
    static VirtualClock& instance();
    
    /**
     * Converts a corrected time, expressed in ticks, into an uncorrected one
     * @param tick corrected ticks
     * @return the uncorrected ticks corresponding
     */
    long long corrected2uncorrected(long long tick){
        return baseComputed+fastNegMul((tick-baseTheoretical),inverseFactorI,inverseFactorD);
    }
        
    /**
     * Converts an uncorrected time, expressed in ticks, into a corrected one
     * @param tick uncorrected ticks
     * @return the corrected ticks corresponding
     */
    long long uncorrected2corrected(long long tick){
        return baseTheoretical+fastNegMul(tick-baseComputed,factorI,factorD);
    }
    
    /**
     * Updates the internal data, used to compute the correction and thus applying it
     * on the provided times.
     * @param baseTheoretical the time that should have expired, since the synchronization
     * started, in theory. So equals to t_0 + kT, where:
     * - t_0 is the first synchronization time point in the local clock, in ticks, corrected only
     *   by VHT and not by FLOPSYNC-2.
     * - k is the number of iterations of the synchronizer.
     * - T is the synchronization interval, as passed in the setSyncPeriod
     * @param baseComputed the time that is expired, in ticks, corrected at each iteration
     * using the raw FLOPSYNC-2 correction. So equals to t_0 + sum_{i=0}^{i=k}(T + corr(i))
     * where corr(i) expresses the correction estimated by the FLOPSYNC-2 controller at the i-th
     * iteration.
     * @param clockCorrection time, expressed in nanoseconds, of the correction computed by FLOPSYNC-2
     * for the current iteration.
     */
    void update(long long baseTheoretical, long long baseComputed, long long clockCorrection);
    
    /**
     * To be used only while initializing the time synchronization. Its purpose is initializing the T value
     * to be used in the correction as proportionality coefficient, thus performing compensation,
     * applying it as skew compensation but including also offset compensation, therefore obtaining
     * a monotonic clock.
     * @param syncPeriod the time T, also known as synchronization interval
     */
    void setSyncPeriod(unsigned long long syncPeriod){
        if(syncPeriod>maxPeriod) throw 0;
        this->syncPeriod=syncPeriod;
    }
    
private:
    VirtualClock(){};
    VirtualClock(const VirtualClock&)=delete;
    VirtualClock& operator=(const VirtualClock&)=delete;
    
    long long fastNegMul(long long a,unsigned int bi, unsigned int bf){
        if(a<0){
            return -mul64x32d32(-a,bi,bf);
        }else{
            return mul64x32d32(a,bi,bf);
        }
    }
    
    //Max period, necessary to guarantee the proper behaviour of runUpdate
    //They are 2^40=1099s
    const unsigned long long maxPeriod=1099511627775;
    unsigned long long syncPeriod=0;
    long long baseTheoretical=0;
    long long baseComputed=0;
    unsigned int factorI=1;
    unsigned int factorD=0;
    unsigned int inverseFactorI=1;
    unsigned int inverseFactorD=0;
};
}

#endif /* VIRTUAL_CLOCK_H */

