/***************************************************************************
 *   Copyright (C) 2016 by Terraneo Federico                               *
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

#include "spi.h"
#include <miosix.h>

using namespace std;

namespace miosix {

Spi& Spi::instance()
{
    static Spi singleton;
    return singleton;
}


unsigned char Spi::sendRecv(unsigned char data)
{
    USART1->TXDATA=data;
    while((USART1->STATUS & USART_STATUS_RXDATAV)==0) ;
    return USART1->RXDATA;
}

void Spi::enable()
{
    USART1->ROUTE=USART_ROUTE_LOCATION_LOC1
                | USART_ROUTE_CLKPEN
                | USART_ROUTE_TXPEN
                | USART_ROUTE_RXPEN;
    USART1->CMD=USART_CMD_CLEARRX
              | USART_CMD_CLEARTX
              | USART_CMD_TXTRIDIS
              | USART_CMD_RXBLOCKDIS
              | USART_CMD_MASTEREN
              | USART_CMD_TXEN
              | USART_CMD_RXEN;
}

void Spi::disable()
{
    USART1->CMD=USART_CMD_TXDIS | USART_CMD_RXDIS;
    USART1->ROUTE=0;
    internalSpi::mosi::mode(Mode::OUTPUT_LOW);
    internalSpi::miso::mode(Mode::INPUT_PULL_DOWN); //To prevent it from floating
    internalSpi::sck::mode(Mode::OUTPUT_LOW);
}

Spi::Spi()
{
    {
        FastInterruptDisableLock dLock;
        CMU->HFPERCLKEN0|=CMU_HFPERCLKEN0_USART1;
    }
    USART1->CTRL=USART_CTRL_MSBF
               | USART_CTRL_SYNC;
    USART1->FRAME=USART_FRAME_STOPBITS_ONE //Should not even be needed
                | USART_FRAME_PARITY_NONE
                | USART_FRAME_DATABITS_EIGHT;
    USART1->CLKDIV=((EFM32_HFXO_FREQ/8000000/2)-1)<<8; //CC2520 max freq is 8MHz
    USART1->IEN=0;
    USART1->IRCTRL=0;
    USART1->I2SCTRL=0;
    //By default leave it disabled, it will be enabled by the PowerManager
    //when the appropriate power domain is enabled
    //enable();
}

}//namespace miosix
