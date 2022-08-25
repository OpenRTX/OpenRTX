/***************************************************************************
 *   Copyright (C) 2013 by Terraneo Federico                               *
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

#ifndef RNG_H
#define RNG_H

#include "interfaces/arch_registers.h"
#include "kernel/sync.h"

namespace miosix {

/**
 * Class to access the hardware random number generator in Miosix
 * Works with the hardware RNG in stm32f2 and stm32f4
 */
class HardwareRng
{
public:
    /**
     * \return an instance of this class (singleton)
     */
    static HardwareRng& instance();
    
    /**
     * \return a 32 bit random number
     * \throws runtime_error if the self test is not passed
     */
    unsigned int get();
    
    /**
     * Fill a buffer with random data
     * \param buf buffer to be filled
     * \param size buffer size
     * \throws runtime_error if the self test is not passed
     */
    void get(void *buf, unsigned int size);
    
private:
    HardwareRng(const HardwareRng&);
    HardwareRng& operator=(const HardwareRng&);
    
    /**
     * Constructor
     */
    HardwareRng() : old(0)
    {
        miosix::FastInterruptDisableLock dLock;
        RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
        RCC_SYNC();
    }
    
    /**
     * \return a 32 bit random number
     * \throws runtime_error if the self test is not passed
     */
    unsigned int getImpl();
    
    /**
     * To save power, enable the peripheral ony when requested
     */
    class PeripheralEnable
    {
    public:
        PeripheralEnable() { RNG->CR=RNG_CR_RNGEN; }
        ~PeripheralEnable() { RNG->CR=0; }
    };
    
    miosix::FastMutex mutex; ///< To protect against concurrent access
    unsigned int old; ///< Previously read value
};

} //namespace miosix

#endif //RNG_H
