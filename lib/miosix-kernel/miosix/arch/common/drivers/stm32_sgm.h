/***************************************************************************
 *   Copyright (C) 2017 by Matteo Michele Piazzolla                        *
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

#define PRESERVE __attribute__((section(".preserve"))) 

namespace miosix {

/**
 * Possible causes for an STM32 reset
 */
enum ResetReason
{
    RST_LOW_PWR=0,
    RST_WINDOW_WDG=1,
    RST_INDEPENDENT_WDG=2,
    RST_SW=3,
    RST_POWER_ON=4,
    RST_PIN=5,
    RST_UNKNOWN=6,
};

/**
 * Driver for the STM32F2 and STM32F4 backup SRAM, here used as
 * SafeGuard Memory, that is, a memory whose value is preseved across resets.
 */
class SGM 
{
public:
    /**
     * \return an instance of this class (singleton)
     */
    static SGM& instance();

    /**
     * Temporarily disable writing to the safeguard memory.
     * By deafult, from reset to when the contrsuctor of this class is called
     * the safeguard memory is not writable. After the constructor is called,
     * the safeguard memory is writable.
     */
    void disableWrite();

    /**
     * Make the safeguard memory writable again, after a call to disableWrite()
     */
    void enableWrite();
    
    /**
     * Return the cause of the last reset of the microcontroller
     */
    ResetReason lastResetReason() { return lastReset; }

private:
    ResetReason lastReset;

    SGM(const SGM&)=delete;
    SGM& operator=(const SGM&)=delete;

    SGM();
    void readResetRegister();
    void clearResetFlag();
};

}
