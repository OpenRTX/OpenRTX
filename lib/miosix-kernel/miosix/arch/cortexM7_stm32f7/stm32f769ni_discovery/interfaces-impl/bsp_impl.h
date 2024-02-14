/***************************************************************************
 *   Copyright (C) 2018 by Terraneo Federico                               *
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

/***************************************************************************
 * bsp_impl.h Part of the Miosix Embedded OS.
 * Board support package, this file initializes hardware.
 ***************************************************************************/

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
 * Called by stage_1_boot.cpp to enable the SDRAM before initializing .data/.bss
 * Requires the CPU clock to be already configured (running from the PLL)
 */
void configureSdram();

/**
 * \internal
 * Board pin definition
 */
typedef Gpio<GPIOJ_BASE,13> userLed1;
typedef Gpio<GPIOJ_BASE,5> userLed2;
typedef Gpio<GPIOA_BASE,12> userLed3;
typedef Gpio<GPIOA_BASE,0> userBtn;
typedef Gpio<GPIOI_BASE,15> sdCardDetect;

inline void ledOn()
{
    userLed1::high();
    userLed2::high();
    userLed3::high();
}

inline void ledOff()
{
    userLed1::low();
    userLed2::low();
    userLed3::low();
}

inline void led1On() { userLed1::high(); }

inline void led1Off() { userLed1::low(); }

inline void led2On() { userLed2::high(); }

inline void led2Off() { userLed2::low(); }

inline void led3On() { userLed3::high(); }

inline void led3Off() { userLed3::low(); }

/**
 * Polls the SD card sense GPIO.
 *
 * This board has no SD card whatsoever, but a card can be connected to the
 * following GPIOs:
 * TODO: never tested
 *
 * \return true. As there's no SD card sense switch, let's pretend that
 * the card is present.
 */
inline bool sdCardSense() { return sdCardDetect::value() == 0; }

/**
\}
*/

}  // namespace miosix

#endif  // BSP_IMPL_H
