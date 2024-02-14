/***************************************************************************
 *   Copyright (C) 2016 by Silvano Seva for Skyward Experimental           *
 *   Rocketry                                                              *
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

#ifndef HWMAPPING_H
#define	HWMAPPING_H

#include "interfaces/gpio.h"

namespace miosix {

//external SPI flash memories, whith dedicated SPI bus
namespace memories {
using sck  = Gpio<GPIOA_BASE, 5>;
using miso = Gpio<GPIOA_BASE, 6>;
using mosi = Gpio<GPIOA_BASE, 7>;
using cs0  = Gpio<GPIOC_BASE,15>;
using cs1  = Gpio<GPIOC_BASE,14>;
using cs2  = Gpio<GPIOC_BASE,13>;
}

//MCP2515 SPI driven CAN interface chip
namespace mcp2515 {
using sck    = Gpio<GPIOB_BASE, 3>;
using miso   = Gpio<GPIOB_BASE, 4>;
using mosi   = Gpio<GPIOB_BASE, 5>;
using tx0rts = Gpio<GPIOB_BASE, 7>;
using tx1rts = Gpio<GPIOB_BASE, 8>;
using tx2rts = Gpio<GPIOB_BASE, 9>;
using interr = Gpio<GPIOA_BASE, 8>;
}

namespace can {
using rx1 = Gpio<GPIOA_BASE,11>;
using tx1 = Gpio<GPIOA_BASE,12>;
using rx2 = Gpio<GPIOB_BASE,12>;
using tx2 = Gpio<GPIOB_BASE,13>;
}

//analog inputs
namespace analogIn {
using ch0 = Gpio<GPIOA_BASE, 4>;
using ch1 = Gpio<GPIOB_BASE, 1>;
using ch2 = Gpio<GPIOB_BASE, 2>;
using ch3 = Gpio<GPIOC_BASE, 5>;
using ch4 = Gpio<GPIOC_BASE, 4>;
using ch5 = Gpio<GPIOC_BASE, 2>;
using ch6 = Gpio<GPIOC_BASE, 6>;
using ch7 = Gpio<GPIOC_BASE, 1>;
using ch8 = Gpio<GPIOC_BASE, 0>;
}

//general purpose IOs (typically used as digital IO)
namespace gpio {
using gpio0 = Gpio<GPIOA_BASE,15>;
using gpio1 = Gpio<GPIOC_BASE, 8>;
using gpio2 = Gpio<GPIOC_BASE, 9>;
using gpio3 = Gpio<GPIOB_BASE,15>;
}

//USART2, connected to RS485 transceiver
namespace usart2 {
using tx  = Gpio<GPIOA_BASE, 2>;
using rx  = Gpio<GPIOA_BASE, 3>;
using rts = Gpio<GPIOA_BASE, 1>;
}

//USART3, connected to RS485 transceiver
namespace usart3 {
using tx  = Gpio<GPIOB_BASE,10>;
using rx  = Gpio<GPIOB_BASE,11>;
using rts = Gpio<GPIOB_BASE,14>;
}

//UART4
namespace uart4 {
using tx  = Gpio<GPIOC_BASE,10>;
using rx  = Gpio<GPIOC_BASE,11>;
}

//UART5
namespace uart5 {
using tx  = Gpio<GPIOC_BASE,12>;
using rx  = Gpio<GPIOD_BASE, 2>;
}

//UART6
namespace uart6 {
using tx  = Gpio<GPIOC_BASE, 6>;
using rx  = Gpio<GPIOC_BASE, 7>;
}
} //namespace miosix

#endif //HWMAPPING_H
