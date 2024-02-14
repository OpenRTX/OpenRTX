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

#ifndef TIMECONVERSION_H
#define TIMECONVERSION_H

namespace miosix {
    
/**
 * \param a 32 bit unsigned number
 * \param b 32 bit unsigned number
 * \return a * b as a 64 unsigned number 
 */
inline unsigned long long mul32x32to64(unsigned int a, unsigned int b)
{
    //Casts are to produce a 64 bit result. Compiles to a single asm instruction
    //in processors having 32x32 multiplication with 64 bit result
    return static_cast<unsigned long long>(a)*static_cast<unsigned long long>(b);
}

/**
 * Multiplication between a 64 bit integer and a 32.32 fixed point number,
 * 
 * The caller must guarantee that the result of the multiplication fits in
 * 64 bits. Otherwise the behaviour is unspecified.
 * 
 * \param a the 64 bit integer number.
 * \param bi the 32 bit integer part of the fixed point number
 * \param bf the 32 bit fractional part of the fixed point number
 * \return the result of the multiplication. The fractional part is discarded.
 */
unsigned long long mul64x32d32(unsigned long long a,
                               unsigned int bi, unsigned int bf) noexcept;

/**
 * This class holds a 32.32 fixed point number used for time conversion
 */
class TimeConversionFactor
{
public:
    /**
     * Default constructor. Leaves the factors unintialized.
     */
    TimeConversionFactor() {}
    
    /**
     * Constructor
     * \param i integer part
     * \param f fractional part
     */
    TimeConversionFactor(unsigned int i, unsigned int f) : i(i), f(f) {}

    /**
     * \return the integer part of the fixed point number
     */
    inline unsigned int integerPart() const { return i; }
    
    /**
     * \return the fractional part of the fixed point number
     */
    inline unsigned int fractionalPart() const { return f; }
    
    /**
     * \param x value to add to the fractional part
     * \return a TimeConversionFactor with the same integer part and the
     * fractional part corrected by delta
     */
    TimeConversionFactor operator+(int delta) const
    {
        return TimeConversionFactor(i,f+delta);
    }
    
private:
    unsigned int i;
    unsigned int f;
};

/**
 * Instances of this class can be used by timer drivers to convert from ticks
 * in the timer resolution to nanoseconds and back.
 */
class TimeConversion
{
public:
    TimeConversion() noexcept;

    /**
     * Constructor
     * Set the conversion factors based on the tick frequency.
     * \param hz tick frequency in Hz. The range of timer frequencies that are
     * supported is 10KHz to 1GHz. The algorithms in this class may not work
     * outside this range
     */
    TimeConversion(unsigned int hz) noexcept;
    
    /**
     * \param tick time point in timer ticks
     * \return the equivalent time point in the nanosecond timescale
     */
    inline long long tick2ns(long long tick) const
    {
        //Negative numbers for tick are not allowed, cast is safe
        auto utick=static_cast<unsigned long long>(tick);
        return static_cast<long long>(convert(utick,toNs));
    }

    /**
     * \param ns time point in nanoseconds
     * \return the equivalent time point in the timer tick timescale
     * 
     * As this function may modify some class variables as part of the
     * internal online adjustment process, it is not reentrant. The caller
     * is responsible to prevent concurrent calls
     */
    long long ns2tick(long long ns) noexcept;
    
    /**
     * \return the conversion factor from ticks to ns
     */
    inline TimeConversionFactor getTick2nsConversion() const { return toNs; }
    
    /**
     * \return the conversion factor from ns to tick
     */
    inline TimeConversionFactor getNs2tickConversion() const { return toTick; }
    
    /**
     * \return the time interval in ns from the last online round trip
     * adjustment for ns2tick() where the adjust offset is cached.
     * This should not matter to you unless you are working on the inner
     * details of the round trip adjustment code, otherwise you can safely
     * ignore this value
     */
    unsigned long long getAdjustInterval() const { return adjustIntervalNs; }
    
    /**
     * \return the cached online round trip adjust offset in ns for ns2tick().
     * This should not matter to you unless you are working on the inner
     * details of the round trip adjustment code, otherwise you can safely
     * ignore this value
     */
    long long getAdjustOffset() const { return adjustOffsetNs; }
    
private:

    /**
     * Compute the error in ticks of an unadjusted conversion from tick to ns
     * and back (a "round trip"), when the toTick conversion coefficient
     * is adjusted by delta and at the given time point.
     * This function is used internally to compute adjust coefficients.
     * \param tick time point in ticks on which to do the round trip
     * \param delta signed value to add the the fractional part of toTick
     * to obtain a temporary coefficient on which the round trip error is
     * computed
     * \return the round trip error in ticks
     */
    long long __attribute__((noinline))
    computeRoundTripError(unsigned long long tick, int delta) const noexcept;

    /**
     * \param x time point to convert
     * \return the converted time point
     */
    static inline unsigned long long convert(unsigned long long x,
                                             TimeConversionFactor tcf)
    {
        return mul64x32d32(x,tcf.integerPart(),tcf.fractionalPart());
    }
    
    /**
     * \param float a floar number
     * \return the number in 32.32 fixed point format
     */
    static TimeConversionFactor __attribute__((noinline))
    floatToFactor(float x) noexcept;

    TimeConversionFactor toNs, toTick;
    unsigned long long adjustIntervalNs, lastAdjustTimeNs;
    long long adjustOffsetNs;
};

} //namespace miosix

#endif //TIMECONVERSION_H
