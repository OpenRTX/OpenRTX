/***************************************************************************
 *   Copyright (C) 2015 by Terraneo Federico                               *
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
* bsp_impl.h Part of the Miosix Embedded OS.
* Board support package, this file initializes hardware.
************************************************************************/

#ifndef BSP_IMPL_H
#define BSP_IMPL_H

#include "interfaces/gpio.h"

namespace miosix {

/**
 * Clear the 4 digit LED display on the board
 */
void clearDisplay();

/**
 * Show a number, in the range -99.9 to 999.9 on the 4 digit LED display
 * \param number the number to show
 */
void showNumber(float number);

/**
 * Show the word "bAt" on the LED display
 */
void showLowVoltageIndicator();

typedef Gpio<GPIOA_BASE,4> cs; ///< For low-level SPI access

/**
 * Send a single byte to SPI, requires to pull cs low first
 * \param x byte to send
 * \return byte received
 */
unsigned char spi1sendRecv(unsigned char x=0);

/**
 * Higher level function to read the status register of the AD7789 connected
 * through SPI
 * \return the status register
 */
unsigned char readStatusReg();

/**
 * Higher level function to read a sample from the AD7789 connected through SPI
 * \return the last converted ADC value
 */
unsigned int readAdcValue();

/**
 * \return true if the supply voltege is high enough
 */
bool lowVoltageCheck();

/**
 * This class allows to store non volatile data into the last FLASH page
 * of the microcontroller.
 */
class NonVolatileStorage
{
public:
    /**
     * \return an instance of this class
     */
    static NonVolatileStorage& instance();
    
    /**
     * \return the maximum size of the available storage
     */
    int capacity() const { return 1024; }
    
    /**
     * Erase the non voltaile storage, resetting it to all 0xff
     * \return true on success, false on failure
     */
    bool erase();
    
    /**
     * Program data into the non volatile storage
     * \param data data to write to the non-volatile storage
     * \param size size of data to write
     * \return true on success, false on failure
     */
    bool program(const void *data, int size);
    
    /**
     * Read back data from the non volatile storage
     * \param data data to read to the non-volatile storage
     * \param size size of data to read
     */
    void read(void *data, int size);

private:
    NonVolatileStorage() {}
    NonVolatileStorage(const NonVolatileStorage&);
    NonVolatileStorage& operator= (const NonVolatileStorage&);
    
    /**
     * Perform the unlock sequence
     * \return true on success, false on failure
     */
    bool IRQunlock();
    
    static const unsigned int baseAddress=0x0801fc00;
};

} //namespace miosix

#endif //BSP_IMPL_H
