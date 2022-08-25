/***************************************************************************
 *   Copyright (C) 2011, 2012 by Terraneo Federico                         *
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

#ifndef SOFTWARE_SPI_H
#define	SOFTWARE_SPI_H

#include "interfaces/gpio.h"

namespace miosix {

/**
 * Software implementation of the SPI protocol mode 0 (CPOL=0, CPHA=0 mode)
 * \param SI an instance of the Gpio class indicating the SPI input pin
 * \param SO an instance of the Gpio class indicating the SPI output pin
 * \param SCK an instance of the Gpio class indicating the SPI clock pin
 * \param CE an instance of the Gpio class indicating the SPI chip enable pin
 * \param numNops number of nops to add to the send loop to slow down SPI clock
 */
template<typename SI, typename SO, typename SCK, typename CE, unsigned numNops>
class SoftwareSPI
{
public:
    /**
     * Initialize the SPI interface
     */
    static void init()
    {
        SI::mode(Mode::INPUT);
        SO::mode(Mode::OUTPUT);
        SCK::mode(Mode::OUTPUT);
        CE::mode(Mode::OUTPUT);
        CE::high();
    }

    /**
     * Send a byte and, since SPI is full duplex, simultaneously receive a byte
     * \param data to send
     * \return data received
     */
    static unsigned char sendRecvChar(unsigned char data);

    /**
     * Send an unsigned short and, since SPI is full duplex, simultaneously
     * receive an unsigned short
     * \param data to send
     * \return data received
     */
    static unsigned short sendRecvShort(unsigned short data);

    /**
     * Send an int and, since SPI is full duplex, simultaneously receive an int
     * \param data to send
     * \return data received
     */
    static unsigned int sendRecvLong(unsigned int data);

    /**
     * Pull CE low, indicating transmission start.
     */
    static void ceLow() { CE::low(); }

    /**
     * Pull CE high, indicating transmission end.
     */
    static void ceHigh() { CE::high(); }
    
private:
    /**
     * Loop used to slow down the SPI as needed
     */
    static void delayLoop();    
};

template<typename SI, typename SO, typename SCK, typename CE, unsigned numNops>
unsigned char SoftwareSPI<SI,SO,SCK,CE,numNops>::
        sendRecvChar(unsigned char data)
{
    unsigned char result=0;
    for(int i=0;i<8;i++)
    {
        if(data & 0x80) SO::high(); else SO::low();
        data<<=1;
        delayLoop();
        SCK::high();
        result<<=1;
        if(SI::value()) result |= 0x1;
        delayLoop();
        SCK::low();     
    }
    return result;
}

template<typename SI, typename SO, typename SCK, typename CE, unsigned numNops>
unsigned short SoftwareSPI<SI,SO,SCK,CE,numNops>::
        sendRecvShort(unsigned short data)
{
    unsigned short result=0;
    for(int i=0;i<16;i++)
    {
        if(data & 0x8000) SO::high(); else SO::low();
        data<<=1;
        delayLoop();
		SCK::high();
        result<<=1;
        if(SI::value()) result |= 0x1;
        delayLoop();
        SCK::low();
    }
    return result;
}

template<typename SI, typename SO, typename SCK, typename CE, unsigned numNops>
unsigned int SoftwareSPI<SI,SO,SCK,CE,numNops>::
        sendRecvLong(unsigned int data)
{
    unsigned int result=0;
    for(int i=0;i<32;i++)
    {
        if(data & 0x80000000) SO::high(); else SO::low();
        data<<=1;
        delayLoop();
        SCK::high();
		result<<=1;
        if(SI::value()) result |= 0x1;
        delayLoop();
        SCK::low();
    }
    return result;
}

template<typename SI, typename SO, typename SCK, typename CE, unsigned numNops>
void SoftwareSPI<SI,SO,SCK,CE,numNops>::delayLoop()
{
    for(int j=0;j<numNops;j++) asm volatile("nop");    
}

} //namespace miosix

#endif  //SOFTWARE_SPI_H
