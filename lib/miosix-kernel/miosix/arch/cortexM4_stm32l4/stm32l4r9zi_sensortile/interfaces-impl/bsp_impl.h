/***************************************************************************
 *   Copyright (C) 2015 by Terraneo Federico                               *
 *   Copyright (C) 2023 by Daniele Cattaneo                                *
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

#include "config/miosix_settings.h"
#include "interfaces/gpio.h"

namespace miosix {

/**
\addtogroup Hardware
\{
*/

/**
 * \internal
 * used by the ledOn() and ledOff() implementation
 */
typedef Gpio<GPIOD_BASE,0> _led;

inline void ledOn()
{
    _led::high();
}

inline void ledOff()
{
    _led::low();
}

typedef Gpio<GPIOE_BASE,1> _greenLed;

inline void greenLedOn()
{
    _greenLed::high();
}

inline void greenLedOff()
{
    _greenLed::low();
}

typedef Gpio<GPIOD_BASE, 4> _usart2_rts;


///\internal Pin connected to SD_DETECT
typedef Gpio<GPIOB_BASE,12> sdCardDetect;

/**
 * Polls the SD card sense GPIO
 * \return true if there is an uSD card in the socket.
 * \note According to the schematic, SD_DETECT is shorted to GND when the card
 * is removed, and open when the card is inserted.
 */
inline bool sdCardSense()
{
    return sdCardDetect::value()!=0;
}

/**
\}
*/

} //namespace miosix

#endif //BSP_IMPL_H
