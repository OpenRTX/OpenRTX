/***************************************************************************
 *   Copyright (C) 2013 by Terraneo Federico and Silvano Seva              *
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

#ifndef STM32F2_I2C_H
#define STM32F2_I2C_H

#include <interfaces/arch_registers.h>
#include "board_settings.h"

namespace miosix {

/**
 * Driver for the I2C1 peripheral in STM32F2 and STM32F4 under Miosix
 */
class I2C1Driver
{
public:
    /**
     * \return an instance of this class (singleton)
     */
    static I2C1Driver& instance();
    
    /**
     * Initializes the peripheral. The only supported mode is 100KHz, master,
     * 7bit address. Note that there is no need to manually call this member
     * function as the constructor already inizializes the I2C peripheral.
     * The only use of this member function is to reinitialize the peripheral
     * if the microcontroller clock frequency or the APB prescaler is changed.
     */
    void init();
    
    /**
     * Send data to a device connected to the I2C bus
     * \param address device address (bit 0 is forced at 0)
     * \param data pointer with data to send
     * \param len length of data to send
     * \param sendStop if set to false disables the sending of a stop condition
     *                 after data transmission has finished
     * \return true on success, false on failure
     */
    bool send(unsigned char address, 
            const void *data, int len, bool sendStop = true);
            
    /**
     * Receive data from a device connected to the I2C bus
     * \param address device address (bit 0 is forced at 1) 
     * \param data pointer to a buffer where data will be received
     * \param len length of data to receive
     * \return true on success, false on failure
     */
    bool recv(unsigned char address, void *data, int len);
    
private:
    I2C1Driver(const I2C1Driver&);
    I2C1Driver& operator=(const I2C1Driver&);
    
    /**
     * Constructor. Initializes the peripheral except the GPIOs, that must be
     * set by the caller to the appropriate alternate function mode prior to
     * creating an instance of this class.
     * \param i2c pinter to the desired I2C peripheral, such as I2C1, I2C2, ...
     */
    I2C1Driver() { init(); }
    
    /**
     * Send a start condition
     * \param address 
     * \param immediateNak
     * \return 
     */
    bool start(unsigned char address, bool immediateNak=false);
    
    /**
     * Wait until until an interrupt occurs during the send start bit and
     * send address phases of the i2c communication.
     * \return true if the operation was successful, false on error
     */
    bool waitStatus1();    
};

} //namespace miosix

#endif //STM32F2_I2C_H
