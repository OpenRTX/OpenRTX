/***************************************************************************
 *   Copyright (C) 2013-2022 by Terraneo Federico and Silvano Seva         *
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

#pragma once

#include <interfaces/gpio.h>

namespace miosix {

/**
 * Driver for the I2C1 peripheral in STM32F2 and STM32F4 under Miosix
 */
class I2C1Master
{
public:
    /**
     * Constructor. This driver only supports master mode with 7bit address.
     * NOTE: do not create multiple instances of this class.
     * \param sda SDA GPIO pin, the constructor configures the pin
     * \param scl SCL GPIO pin, the constructor configures the pin
     * \param frequency I2C bus frequency, in kHz. Officially only 100kHz and
     * 400kHz are supported, but some overclocking may be possible.
     */
    I2C1Master(GpioPin sda, GpioPin scl, int frequency=100);

    /**
     * Send data
     * - send START condition
     * - send address
     * - send data
     * - send STOP condition
     * 
     * \param address device address, stored in bits 7 to 1. Bit 0 is ignored
     * \param data pointer with data to send
     * \param len length of data to send
     * \return true on success, false on failure
     */
    bool send(unsigned char address, const void *data, int len)
    {
        return send(address,data,len,true);
    }

    /**
     * Purely receive data
     * - send START condition
     * - send address
     * - receive data
     * - send STOP condition
     * 
     * \param address device address, stored in bits 7 to 1. Bit 0 is ignored
     * \param data pointer to a buffer where data will be received
     * \param len length of data to receive
     * \return true on success, false on failure
     */
    bool recv(unsigned char address, void *data, int len);

    /**
     * Send and receive data, with a repeated START betwwen send and receive
     * - send START condition
     * - send address
     * - send data
     * - send repeated START
     * - send address
     * - receive data
     * - send STOP condition
     * 
     * \param address device address, stored in bits 7 to 1. Bit 0 is ignored
     * \param txData data to transmit, set to nullptr if none
     * \param txLen number of bytes to transmit, set to 0 if none
     * \param rxData data to receive, set to nullptr if none
     * \param rxLen number of bytes to receive, set to 0 if none
     */
    bool sendRecv(unsigned char address, const void *txData, int txLen,
                  void *rxData, int rxLen)
    {
        if(send(address,txData,txLen,false)==false) return false;
        return recv(address,rxData,rxLen);
    }

    /**
     * Probe if a device is on the bus
     * - send START condition
     * - send address
     * - send STOP condition
     * \return true if the address was acknowledged on the bus
     */
    bool probe(unsigned char address)
    {
        bool result=start(address & 0xfe);
        stop();
        return result;
    }
    
    /**
     * Destructor
     */
    ~I2C1Master();
    
private:
    I2C1Master(const I2C1Master&);
    I2C1Master& operator=(const I2C1Master&);
    
    /**
     * Internal version of send, able to omit the final STOP
     * \param address device address (bit 0 is forced at 0)
     * \param data pointer with data to send
     * \param len length of data to send
     * \param sendStop if set to false disables the sending of a STOP condition
     * after data transmission has finished
     * \return true on success, false on failure
     */
    bool send(unsigned char address, const void *data, int len, bool sendStop);
    
    /**
     * Send a start condition
     * \param address device address (includes r/w bit)
     * \return true if successful
     */
    bool start(unsigned char address);
    bool startWorkaround(unsigned char address, int len);
    
    /**
     * Wait until until an interrupt occurs during the send start bit and
     * send address phases of the i2c communication.
     * \return true if the operation was successful, false on error
     */
    bool waitStatus1();
    
    /**
     * Send a stop condition, waiting for its completion
     */
    void stop();

    static bool checkMultipleInstances;
};

} //namespace miosix
