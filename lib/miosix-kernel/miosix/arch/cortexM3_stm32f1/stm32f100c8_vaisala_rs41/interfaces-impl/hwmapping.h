/***************************************************************************
 *   Copyright (C) 2020 by Silvano Seva                                    *
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

#include "interfaces/gpio.h"

namespace miosix {

typedef Gpio<GPIOA_BASE, 5>  vbat;
typedef Gpio<GPIOA_BASE, 6>  pushbutton;
typedef Gpio<GPIOA_BASE, 12> poweroff;

typedef Gpio<GPIOB_BASE, 7> greenLed;
typedef Gpio<GPIOB_BASE, 8> redLed;

namespace frontend
{
    typedef Gpio<GPIOA_BASE, 1> meas_out;
    typedef Gpio<GPIOA_BASE, 2> pullup_hygro;
    typedef Gpio<GPIOA_BASE, 3> spst2_2;
    typedef Gpio<GPIOA_BASE, 4> frontend_unk_1;
    typedef Gpio<GPIOA_BASE, 7> heat_hum_1;

    typedef Gpio<GPIOB_BASE, 1> frontend_unk_2;
    typedef Gpio<GPIOB_BASE, 3> spdt1_2;
    typedef Gpio<GPIOB_BASE, 4> spdt2_2;
    typedef Gpio<GPIOB_BASE, 5> spdt3_2;
    typedef Gpio<GPIOB_BASE, 6> spst1_2;
    typedef Gpio<GPIOB_BASE, 9> heat_hum_2;

    typedef Gpio<GPIOB_BASE, 12> pullup_temp;

    typedef Gpio<GPIOC_BASE, 14> spst3_2;
    typedef Gpio<GPIOC_BASE, 15> spst4_2;
}

namespace nfc
{
    typedef Gpio<GPIOA_BASE, 11> in1;
    typedef Gpio<GPIOA_BASE, 0>  in2;
    typedef Gpio<GPIOB_BASE, 0>  out;
}

namespace gps
{
    typedef Gpio<GPIOA_BASE, 9>  rxd;
    typedef Gpio<GPIOA_BASE, 10> txd;
    typedef Gpio<GPIOA_BASE, 15> nReset;
}

namespace spi
{
    typedef Gpio<GPIOB_BASE, 13> sclk;
    typedef Gpio<GPIOB_BASE, 14> miso;
    typedef Gpio<GPIOB_BASE, 15> mosi;

    typedef Gpio<GPIOA_BASE, 8>  csBaro;
    typedef Gpio<GPIOB_BASE, 2>  csEeprom;
    typedef Gpio<GPIOC_BASE, 13> csRadio;
};

} //namespace miosix
