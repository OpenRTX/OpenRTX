/***************************************************************************
 *   Copyright (C) 2014 by Terraneo Federico                               *
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

#ifndef SERVO_STM32_H
#define	SERVO_STM32_H

#include "miosix.h"

namespace miosix {

/**
 * This class is designed to drive up to 4 servomotors. It generates
 * four square waves that are synchronized with respect to each other,
 * and allows the execution of code that is too synchronized with the
 * waveform generation, to ease the development of closed loop control
 * code using the servos as actuators.
 * This class can be safely accessed by multiple threads, except the
 * waitForCycleBegin() member function.
 */
class SynchronizedServo
{
public:
    /**
     * \return an instance of the SynchronizedServo class (singleton)
     * When first returned, the servo waveform generation is stopped. You must
     * enable at least one channel call start() and setPosition() before the
     * servo driving waveforms will be generated.
     */
    static SynchronizedServo& instance();
    
    /**
     * Enable a channel. Can only be called with the outputs stopped. Even if
     * the channel is enabled, when it is started it will not produce any
     * waveform until the first call to setPosition()
     * \param channel which channel to enable, must be between 0 and 3.
     */
    void enable(int channel);
    
    /**
     * Disable a channel. Can only be called with the outputs stopped.
     * \param channel which channel to disable, must be between 0 and 3.
     */
    void disable(int channel);
    
    /**
     * Set the servo position. Can only be called with the outputs started.
     * The written value takes effect at the next waveform generation cycle.
     * \param channel channel whose output need to be changed, between 0 and 3
     * \param value a float value from 0.0 to 1.0. Due to the limited timer
     * resolution, the actual set value is approximated. With the default values
     * of waveform frequency, min and max width the range between 0.0 and 1.0
     * is covered by around 3200 points.
     */
    void setPosition(int channel, float value);
    
    /**
     * \param channel channel whose output need to be read, between 0 and 3
     * \return the exact servo position, considering the approximations.
     * For this reason, the returned value may differ from the last call to
     * setPosition(). NAN is returned if no setValue() was called on the channel
     * since the last call to start()
     */
    float getPosition(int channel);
    
    /**
     * Start producing the output waveforms.
     */
    void start();
    
    /**
     * Stop producing the output waveforms. If a thread is waiting in
     * waitForCycleBegin() it will be forecefully woken up.
     * As a side effect, the position of all the channels will be set to NAN.
     * When you restart the timer, you must call setPosition() on each enabled
     * channel before the channel will actually produce a waveform.
     * This function waits until the end of a waveform generation cycle in order
     * to not produce glitches. For this reason, it may take up to
     * 1/getFrequency() to complete, which with the default value of 50Hz is 20ms
     */
    void stop();
    
    /**
     * Wait until the begin of a waveform generation cycle
     * \return false if a new cycle of waveform generation has begun, or true
     * if another thread called stop(). Only one thread at a time can call this
     * member function. If more than one thread calls this deadlock will occur
     * so don't do it!
     */
    bool waitForCycleBegin();
    
    /**
     * Set the frequency of the generated waveform. Can only be called
     * with the outputs stopped. The default is 50Hz. Note that due to prescaler
     * resolution, the actual frequency is set to the closest possible value.
     * To know the actual frequency, call getFrequency()
     * \param frequency desired servo update frequency in Hz
     * Must be between 10 and 100Hz
     */
    void setFrequency(unsigned int frequency);
    
    /**
     * \return the actual servo update frequency in Hz. Note that the
     * returned value is floating point as the returned frequency takes into
     * account approximations due to the prescaler resolution
     */
    float getFrequency() const;
    
    /**
     * Set the minimum width of the generated pulses, that is, the pulse width
     * generated when an output is set to zero with setPosition(x,0).
     * The default is 1000us. Can only be called with the outputs stopped.
     * \param minPulse minimum pulse width in microseconds.
     * Must be between 500 and 1300.
     */
    void setMinPulseWidth(float minPulse);
    
    /**
     * \return minimum pulse width in microseconds
     */
    float getMinPulseWidth() const { return minWidth*1e6f; }
    
    /**
     * Set the maximum width of the generated pulses, that is, the pulse width
     * generated when an output is set to one with setPosition(x,1).
     * The default is 2000us. Can only be called with the outputs stopped.
     * \param maxPulse maximum pulse width in microseconds.
     * Must be between 1700 and 2500.
     */
    void setMaxPulseWidth(float maxPulse);
    
    /**
     * \return maximum pulse width in microseconds
     */
    float getMaxPulseWidth() const { return maxWidth*1e6f; }
    
private:
    SynchronizedServo(const SynchronizedServo&);
    SynchronizedServo& operator= (const SynchronizedServo&);
    
    /**
     * Constructor
     */
    SynchronizedServo();
    
    /**
     * Precompute a and b coefficient to make setPosition() faster
     */
    void precomputeCoefficients();
    
    /**
     * \return the input frequency of the timer prescaler
     */
    static unsigned int getPrescalerInputFrequency();
    
    /**
     * Wait until the timer overflows from 0xffff to 0. Can only be called with
     * interrupts disabled
     */
    static void IRQwaitForTimerOverflow(FastInterruptDisableLock& dLock);
    
    float minWidth, maxWidth; ///< Minimum and maximum pulse widths
    float a, b;               ///< Precomputed coefficients
    FastMutex mutex;          ///< Mutex to protect from concurrent access
    enum {
        STOPPED,  ///< Timer is stopped
        STARTED   ///< Timer is started
    } status;
};

} //namespace miosix

#endif //SERVO_STM32_H
