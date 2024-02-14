/***************************************************************************
 *   Copyright (C) 2015, 2016 by Terraneo Federico                         *
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

#include "timeconversion.h"
#include <limits>

#ifdef TEST_ALGORITHM

#include <iostream>
#include <cassert>
#include <vector>
#include <cmath>

static bool print=true;
#define P(x) if(print) std::cout<<#x<<'='<<x<<' ';
#define NL if(print) std::cout<<std::endl;
#define ITERATION if(print) std::cout<<'+';

#endif //TEST_ALGORITHM

namespace miosix {

/**
 * \param x 64 bit unsigned number
 * \return 32 unsigned number with the lower 32 bits of x
 */
static inline unsigned int lo(unsigned long long x) { return x & 0xffffffff; }

/**
 * \param x 64 bit unsigned number
 * \return 32 unsigned number with the upper 32 bits of x
 */
static inline unsigned int hi(unsigned long long x) { return x>>32; }

unsigned long long mul64x32d32(unsigned long long a,
                               unsigned int bi, unsigned int bf) noexcept
{
    /*
     * The implemntation is a standard multiplication algorithm:
     *                               | 64bit         | 32bit         | Frac part
     * ----------------------------------------------------------------------
     *                               |  hi(a)        | lo(a)         | 0    x
     *                               |               | bi            | bf   =
     * ======================================================================
     *               |  hi(bi*lo(a)) |  lo(bi*lo(a)) | 0             | -
     *  hi(bi*hi(a)) | +lo(bi*hi(a)) |               |               |
     *               |               | +hi(bf*lo(a)) |  lo(bf*lo(a)) | 0
     *               | +hi(bf*hi(a)) | +lo(bf*hi(a)) |               |
     * ----------------------------------------------------------------------
     *  96bit        | 64bit         | 32bit         | Frac part
     *  (Discarded)  | <------ returned part ------> | (Disacrded)
     * 
     * Note that [hi(bi*lo(a))|lo(bi*lo(a))] and [hi(bf*hi(a))|lo(bf*hi(a))]
     * are two 64 bit numbers with the same alignment as the result, so
     * result can be rewritten as
     * bi*lo(a) + bf*hi(a) + hi(bf*lo(a)) + lo(bi*hi(a))<<32
     * 
     * The arithmetic rounding is implemented by adding one to the result if
     * lo(bf*lo(a)) has bit 31 set. That is, if the fractional part is >=0.5.
     * 
     * This code (without arithmetic rounding) takes around 30..40 clock cycles
     * on Cortex-A9 (arm/thumb2), Cortex-M3 (thumb2), Cortex-M4 (thumb2),
     * ARM7TDMI (arm), with GCC 4.7.3 and optimization levels -Os, -O2, -O3.
     * 
     * TODO: this code is *much* slower with architectures missing a 32x32
     * multiplication with 64 bit result, probably in the thousands of clock
     * cycles. An example arch is the Cortex-M0. How to deal with those arch?
     */

    unsigned int aLo=lo(a);
    unsigned int aHi=hi(a);

    unsigned long long result=mul32x32to64(bi,aLo);
    result+=mul32x32to64(bf,aHi);

    unsigned long long bfaLo=mul32x32to64(bf,aLo);
    result+=hi(bfaLo);

    //Uncommenting this adds arithmetic rounding (round to nearest value).
    //Leaving it commented out always rounds towards lowest and makes the
    //algorithm significantly faster by not requiring a branch
    //if(bfaLo & 0x80000000) result++;

    //This is a 32x32 -> 32 multiplication (upper 32 bits disacrded), with
    //the 32 bits shifted to the upper word of a 64 bit number
    result+=static_cast<unsigned long long>(bi*aHi)<<32;

    //Caller is responsible to never call this with values that produce overflow
    return result;
}

/**
 * \param x a long long
 * \return true if x>=0
 */
static inline bool sign(long long x)
{
    return x>=0;
}

/**
 * \param x a long long
 * \return |x|
 * 
 * Note that compared to llabs this function returns an unsigned number, and
 * is also sure to be inlined
 */
static inline unsigned long long uabs(long long x)
{
    long long result= sign(x) ? x : -x;
    return static_cast<unsigned long long>(result);
}

/*
 * A note on the issues of tick to nanosecond conversion and back.
 * Converting a number in tick to nanoseconds requires multiplying the ticks by
 * a coefficient M, while doing the opposite requires a multiplication by 1/M.
 * This code assumes that the tick frequency is less than 1GHz, so that M > 1,
 * and thus 1/M < 1. Doing an exact multiplication would be too expensive
 * computationally, so the coefficients are rounded in a 32.32 fixed point
 * representation. In this representation, numbers are in the form
 * A+B/2^32, where A and B are two unsigned 32 bit numbers.
 * 
 * The essential requirements on this approximation are two:
 * - the error of the conversion from tick to nanosecond should be as small
 *   as possible
 * - if a value in ticks is converted to nanoseconds and then converted back to
 *   ticks, the result in ticks should be again the original value in ticks
 *   that we started with, with a very small error (say less than two ticks).
 *   For this reason, the error for the conversion from nanosecond to tick
 *   should be equal and opposite in sign to the tick to nanosecond error.
 * 
 * The reason for the first requirement is easy to understand, the tick to
 * nanosecond conversion is used when reading the hardware clock, so errors
 * in this conversion result in errors in time measurement.
 * Also, this requirement is easy to achieve with the given representation,
 * as the M coefficient is greater than 1, and a fixed point representation
 * has a high resolution when storing large numbers. 
 * Tests done with TEST_ALGORITHM have shown that the tick to nanosecond is
 * zero for some relevant frequencies, and is generally below 0.04ppm,
 * or an error that accumulates at a rate of 1.26 seconds per year of uptime,
 * which is lower than that of most clock crystals.
 * 
 * The second requirement requires some more explanation. Assume a typical use
 * case for the timing subsystem: a task gets the current time, adds a small
 * delay, say 1ms, and does an absolute sleep till that time point.
 * The former operation, getting the time, requires a tick to nanosecond
 * conversion, while the latter a nanosecond to tick for setting the interrupt.
 * If these two conversions, that are here called a "round trip" would introduce
 * an error of many ticks, that would make the sleep imprecise. At first
 * this may seem a non-issue, as the errors are in the parts per million range,
 * but the key point is that we are working with absolute times. So after
 * one year of uptime, a mere mismatch of 0.04ppm between the two
 * conversion coefficients will make our 1ms sleep become a 1261ms one,
 * or -depending on the error sign- a sleep in the past!
 * 
 * This issue is also difficult to solve, as the back conversion coefficient is
 * less than 1, and the fixed point stores small numbers with less resolution,
 * so the error of the nanosecond to tick conversion can grow as large as
 * 10ppm for tick frequencies in the 10KHz range. The solution to this problem
 * is twofold. First, an offline optimization of the back conversion coefficient
 * is done int the TimeConversion constructor, with the aim of having its error
 * as much as possible equal and opposite in sign to the tick to nanosecond
 * error.
 * Then, an online round trip adjustment is done in ns2tick(), by computing the
 * round trip offset and subtracting it to the conversion, in order to make
 * sure the round trip error is less than two ticks.
 * As the online adjustment is a little expensive, the result is cached, and for
 * conversion which are "near" to the one at the previous nanosecond to tick
 * call the same offset is used. The range that is considered "near" is
 * again computed offline, as it depends on how good the offline optimization
 * was at making one conversion coefficient the opposite of the other, but is
 * generally in the range of a few seconds to a few tens of seconds.
 */

//
// class TimeConversion
//

long long TimeConversion::ns2tick(long long ns) noexcept
{
    /*
     * This algorithm does the online adjustment to compensate for round trip
     * error that accumulates over time. It is probably the least intuitive
     * algorithm in this file, so it requires careful explanation.
     * If we are close enough to te last adjustment we simply perform the
     * conversion and add adjustOffsetNs, which is the cached value. Otherwise,
     * we need to recompute the offset. This part is a straightforward caching.
     * 
     * The first implementation of the recomputation was a simple
     * 
     * adjustOffsetNs=ns-convert(convert(ns,toTick),toNs);
     * lastAdjustTimeNs=ns;
     * 
     * which is straightforward, but it failed when tested with very long
     * ns values (100+ years) and low tick frequencies (8MHz or less).
     * The reason for this is that the round trip computation first converts
     * the given ns value to ticks, which incurs in the error that we want to
     * get rid of, and then converts it back to nanoseconds, resulting in a
     * value that we call ns2. However, since we are adjusting the
     * first conversion to be equal to the second one, the adjust value so
     * obtained is good for compensating the error in a range of amplitude
     * adjustIntervalNs around ns2, NOT around ns!
     * So, if the error grows so large that ns2-ns is greater than
     * adjustIntervalNs, the simple round trip written above does not zero
     * the error. To solve this, an iterative algorithm is needed.
     * 
     * Somewhere else    ns2tick
     * in the kernel
     * 
     *     tick          tick2 --+
     *      |             ^      |
     *      |             |      |
     *      V             |      V
     *      ns -----------+      ns2
     * 
     * The following diagram explains it further. Soewhere else in the kernel
     * a value in ticks was read from the hardware counter, and converted to ns.
     * Within ns2tick we don't know what tick is, and if we're doing a round
     * trip adjustment we start by back converting ns, resulting in tick2
     * which is different from tick, because the point of all this is that
     * the conversion from ns to tick is imprecise. If we then complete the
     * round trip and naively compute adjustOffsetNs as the difference from
     * ns2 and ns, the resulting adjustment when applied to ns will only
     * give us the correct answer, tick, if the difference between ns and ns2
     * is less than adjustIntervalNs.
     * 
     * The algorithm that fixes this works as follows. Before the for loop,
     * there's an adjustOffsetNs=0 statement.
     * At the first iteration we try to do a round trip around
     * ns+adjustOffsetNs, but adjustOffsetNs is zero, so we get the same result
     * as the previous round trip described above. As the result of this
     * round trip gets us ns2, which as said earlier is the center of the
     * validity range for the round trip, we store it in our lastAdjustTimeNs
     * variable. Then we compute the adjust offset as the original time point,
     * ns+adjustOffsetNs, minus the round trip value ns2.
     * Then we check the validity range of our find. The check is whether we
     * are in a range which is half the size of our real validity range. This
     * is a compromise, as using the full validity range would lower the number
     * of iterations of the algorithm that will produce a correct answer, but
     * will diminish the value of caching, as ns2tick will be likely called with
     * values close to the previous ns one, and if we lastAdjustTimeNs compared
     * to ns is at edge of the range, later calls are likely to require a
     * readjustment. To utilize the caching range fully, we would ideally want
     * to stop when ns==lastAdjustTimeNs, but this will require too many
     * iterations and would slow down the algorithm.
     * Anyway, if a single iteration isn't enough, we iterate again, using
     * the previous adjustOffsetNs as a guess to get make tick2 closer to
     * tick this time. The maximum number of iterations that was observed is
     * just 3, so the iteration is particularly efficient.
     * As you can see, the for loop has a hard limit of 5 iterations. This is 
     * NOT expected to occur in practice, and has only been done to be 100%
     * sure that this algorithm won't turn into an infinite loop when given
     * off-design numbers, such as negative numbers.
     * The last thing worth mentioning is that a previous implementation omitted
     * the adjustOffsetNs=0 before the for loop. As explained, each iteration
     * works by using the previous adjustOffsetNs as a guess for the next one,
     * and the initializtion of was omitted to use the previous value as a guess
     * for the new adjustment. While this was shown to reduce the number of
     * iterations in some cases, if ns2tick() was first called with a very large
     * value, and then with a very small one, a negative adjustOffsetNs would
     * result in underflow and caused wrong results to bbe produced.
     * Also, the casts to unsigned are needed because ns+adjustOffsetNs was
     * shown to overflow for values of ns close to the maximum number a
     * long long can hold and positive adjustOffsetNs, but uns+adjustOffsetNs
     * can hold positive numbers far greater than its signed couterpart.
     * 
     * This algorithm was benchmarked on an efm32 Cortex-M4 microcontroller
     * running at 48MHz, and this is the runtime:
     * using cached value     115 cycles
     * readjust 1 iteration   284 cycles
     * readjust 2 iterations  438 cycles
     */

    //Negative numbers for ns are not allowed, cast is safe
    auto uns=static_cast<unsigned long long>(ns);
    if(uabs(static_cast<long long>(uns-lastAdjustTimeNs))>adjustIntervalNs)
    {
        adjustOffsetNs=0;
        for(int i=0;i<5;i++)
        {
            #ifdef TEST_ALGORITHM
            ITERATION;
            #endif //TEST_ALGORITHM
            lastAdjustTimeNs=convert(convert(uns+adjustOffsetNs,toTick),toNs);
            adjustOffsetNs=static_cast<long long>((uns+adjustOffsetNs)-lastAdjustTimeNs);
            if(uabs(static_cast<long long>(uns-lastAdjustTimeNs))<(adjustIntervalNs>>1)) break;
        }
        #ifdef TEST_ALGORITHM
        NL;
        #endif //TEST_ALGORITHM
    }
    return static_cast<long long>(convert(uns+adjustOffsetNs,toTick));
}

TimeConversion::TimeConversion() noexcept
    : toNs(1,0), toTick(1,0),
      adjustIntervalNs(std::numeric_limits<unsigned long long>::max()),
      lastAdjustTimeNs(0), adjustOffsetNs(0)
{}

TimeConversion::TimeConversion(unsigned int hz) noexcept
    : lastAdjustTimeNs(0), adjustOffsetNs(0)
{
    //
    // As a first part, compute the initial toNs and toTick coefficients
    //
    float hzf=static_cast<float>(hz);
    toNs=floatToFactor(1e9f/hzf);
    toTick=floatToFactor(hzf/1e9f);


    //
    // Then perform the bisection algorithm to optimize toTick offline
    //

    /*
     * For choosing the time point on which to do the toTick coefficient
     * optimization, we need both an as high as possible number, but also
     * without fear of overflow. 1<<62 is ~146years, but even if during the
     * bisection the number nearly doubles, no overflow occurs.
     */
    const unsigned long long longUptimeNs=1ULL<<62;
    const unsigned long long longUptimeTick=convert(longUptimeNs,toTick);
    /*
     * Max correction 25ppm (1/400000=25ppm). This value is a guess based on
     * the observed correction values when running with TEST_ALGORITHM, and has
     * the advantage that the number of iterations is lower when hz is lower,
     * thus being less time consuming on slow CPUs
     */
    int aDelta=toTick.fractionalPart()/40000;
    int bDelta=-aDelta;
    long long aError=computeRoundTripError(longUptimeTick,aDelta);
    long long bError=computeRoundTripError(longUptimeTick,bDelta);
    if(sign(aError)!=sign(bError))
    {
        //Different sign, do a binary search
        for(;;)
        {
            #ifdef TEST_ALGORITHM
            ITERATION;
            #endif //TEST_ALGORITHM
            int cDelta=(aDelta+bDelta)/2;
            if(cDelta==aDelta || cDelta==bDelta) break;
            long long cError=computeRoundTripError(longUptimeTick,cDelta);
            if(sign(aError)==sign(cError))
            {
                aDelta=cDelta;
                aError=cError;
            } else {
                bDelta=cDelta;
                bError=cError;
            }
        }
    }
    int delta=uabs(aError)<uabs(bError) ? aDelta : bDelta;
    toTick=toTick+delta;
    #ifdef TEST_ALGORITHM
    NL;
    unsigned int maxCorrection=toTick.fractionalPart()/40000;
    P(maxCorrection);
    P(delta);
    NL;
    #endif //TEST_ALGORITHM

    //
    // Finally find the period at which the online adjustment needs to be done
    //

    /*
     * Based on running with TEST_ALGORITHM it looks like the offline
     * optimization is quite good, and the round trip error remains less than
     * two ticks for 4 seconds or more in most cases. We do not have to be ultra
     * precise in optimizing this period, just a rough value is good enough.
     * For this reason, we start with a first guess of 128s, and divide it by
     * two until the error is less than two ticks. We also stop after 9 tries,
     * that is when the period is down to 0.25s. After that, we unconditionally
     * divide it again by 2, as tests shows that otherwise the error can
     * still grow to 2 ticks in some cases, so the final range is 0.125s .. 64s.
     * Adding a limit to the number of iterations is because a period less than
     * the minimum is NOT expected to occur, and it was done to prefer reducing
     * the online correction overhead at the price of a higher error for corner
     * case values (if they even exist and are practically relevant).
     */
    adjustIntervalNs=128000000000ULL;
    unsigned long long tick=convert(adjustIntervalNs,toTick);
    for(int i=0;i<9;i++)
    {
        if(uabs(computeRoundTripError(tick,0))<2) break;
        tick>>=1;
        adjustIntervalNs>>=1;
    }
    adjustIntervalNs>>=1;
    #ifdef TEST_ALGORITHM
    double adjustInterval=static_cast<double>(adjustIntervalNs)/1e9;
    P(adjustInterval);
    NL;
    #endif //TEST_ALGORITHM
    
    /*
     * This constructor was benchmarked on an efm32 Cortex-M4 microcontroller
     * running at 48MHz, and this is the runtime:
     * tick freq 32768   2458 cycles
     * tick freq 400MHz  4800 cycles
     */
}

long long TimeConversion::computeRoundTripError(unsigned long long tick,
                                                int delta) const noexcept
{
    auto adjustedToTick=toTick+delta;
    unsigned long long ns=convert(tick,toNs);
    unsigned long long roundTrip=convert(ns,adjustedToTick);
    return static_cast<long long>(tick-roundTrip);
}

TimeConversionFactor TimeConversion::floatToFactor(float x) noexcept
{
    const float twoPower32=4294967296.f; //2^32 as a float
    unsigned int i=x;
    unsigned int f=(x-i)*twoPower32;
    return TimeConversionFactor(i,f);
}

} //namespace miosix

//Testsuite for multiplication algorithm and factor computation. Compile with:
// g++ -std=c++14 -O2 -DTEST_ALGORITHM -o test timeconversion.cpp; ./test
#ifdef TEST_ALGORITHM

using namespace std;
using namespace miosix;

void printRoundTripError(TimeConversion& tc, long long tick)
{
    long long roundTripError=tick-tc.ns2tick(tc.tick2ns(tick));
    P(roundTripError);
    NL;
}

long double coefficient(unsigned int bi, unsigned int bd)
{
    //Recompute rounded value using those coefficients. The aim of this test
    //is to assess the multiplication algorithm, so we use integer and
    //fractional part back-converted to a long double that contains the exact
    //same value as the fixed point number
    const long double twoPower32=4294967296.; //2^32 as a long double
    return static_cast<long double>(bd)/twoPower32+static_cast<long double>(bi);
}

void test(double b, unsigned int bi, unsigned int bd, int iterations)
{
    long double br=coefficient(bi,bd);
    P(bi);
    P(bd);
    //This is the error of the 32.32 representation and factor computation
    double errorPPM=(b-br)/b*1e6;
    P(errorPPM);

    srand(0);
    for(int i=0;i<iterations;i++)
    {
        unsigned int aHi=rand() & 0xfff;
        unsigned int aLo=rand() & 0xffffffff;
        unsigned long long a=static_cast<unsigned long long>(aHi)<<32
                           | static_cast<unsigned long long>(aLo);
        unsigned long long result=mul64x32d32(a,bi,bd);
        //Our reference is a long double multiplication
        unsigned long long reference=static_cast<long double>(a)*br;
        assert(uabs(result-reference)<2);
    }
    NL;
}

void testns2tick(TimeConversion& tc, int iterations)
{
    //First, we get the tick value that results in the maximum ns value
    //that fits in a long long
    unsigned int bi,bd;
    bi=tc.getTick2nsConversion().integerPart();
    bd=tc.getTick2nsConversion().fractionalPart();
    long double toNs=coefficient(bi,bd);
    long long maxTick=numeric_limits<long long>::max()/toNs;
    //Care about rounding
    while(tc.tick2ns(maxTick)<0) maxTick--;
    print=false;
    srand(0);

    //Fully random test
    for(int i=0;i<iterations;i++)
    {
        long long a=static_cast<unsigned long long>(rand() & 0x7fffffff)<<32
                  | static_cast<unsigned long long>(rand());
        a=a % maxTick;
        assert(uabs(tc.ns2tick(tc.tick2ns(a))-a)<2);
    }
    //Large and small test, was proven to check some corner cases
    for(int i=0;i<iterations;i++)
    {
        long long a=static_cast<unsigned long long>(rand() & 0x7fffffff)<<32
                  | static_cast<unsigned long long>(rand());
        a=a % maxTick;
        assert(uabs(tc.ns2tick(tc.tick2ns(a))-a)<2);

        long long b=static_cast<unsigned long long>(rand());
        b=b % maxTick;
        assert(uabs(tc.ns2tick(tc.tick2ns(b))-b)<2);
    }
    //Largest and small test
    for(int i=0;i<iterations;i++)
    {
        assert(uabs(tc.ns2tick(tc.tick2ns(maxTick))-maxTick)<2);

        long long b=static_cast<unsigned long long>(rand());
        b=b % maxTick;
        assert(uabs(tc.ns2tick(tc.tick2ns(b))-b)<2);
    }

    print=true;
}

int main()
{
    vector<double> freqs={10000, 32768, 100000, 1e6, 8e6, 24e6, 48e6, 72e6,
                          84e6, 120e6, 168e6, 180e6, 400e6};
    for(double d : freqs)
    {
        cout<<"==== "<<d<<" ===="<<endl;
        TimeConversion tc(d);
        double b;
        unsigned int bi, bd;
        
        long long after250year=250LL * 365LL * 24LL * 3600LL * 1000000000LL;
        long long tick=tc.ns2tick(after250year);
        printRoundTripError(tc,tick);
        //Check the error at the edges of the cached value
        double delta=static_cast<double>(tc.getAdjustInterval())/1e9*d*0.49;
        printRoundTripError(tc,tick-delta);
        printRoundTripError(tc,tick+delta);

        b=1e9/d;
        bi=tc.getTick2nsConversion().integerPart();
        bd=tc.getTick2nsConversion().fractionalPart();
        test(b,bi,bd,1000000);

        b=d/1e9;
        bi=tc.getNs2tickConversion().integerPart();
        bd=tc.getNs2tickConversion().fractionalPart();
        test(b,bi,bd,1000000);

        testns2tick(tc,1000000);
        cout<<endl;
    }
}

#endif //TEST_ALGORITHM
