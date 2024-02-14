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

#ifndef HWMAPPING_H
#define	HWMAPPING_H

#include "interfaces/gpio.h"

namespace miosix {

typedef Gpio<GPIOC_BASE,11> led;

namespace expansion {
typedef Gpio<GPIOA_BASE, 0> gpio2;
typedef Gpio<GPIOA_BASE, 1> gpio3;
typedef Gpio<GPIOA_BASE, 2> gpio4;
typedef Gpio<GPIOA_BASE, 5> spi1sck;
typedef Gpio<GPIOA_BASE, 7> spi1mosi;
typedef Gpio<GPIOB_BASE, 4> spi1miso;
typedef Gpio<GPIOA_BASE,15> spi1cs;
typedef Gpio<GPIOB_BASE,13> spi2sck;
typedef Gpio<GPIOC_BASE, 3> spi2mosi;
typedef Gpio<GPIOC_BASE, 2> spi2miso;
typedef Gpio<GPIOB_BASE,12> spi2cs;

}

namespace button {
typedef Gpio<GPIOE_BASE, 2> b1;
typedef Gpio<GPIOE_BASE, 3> b2;
typedef Gpio<GPIOE_BASE, 4> b3;
typedef Gpio<GPIOE_BASE, 5> b4;
}

namespace usb {
typedef Gpio<GPIOB_BASE,14> dm;
typedef Gpio<GPIOB_BASE,15> dp;
}

namespace display {
typedef Gpio<GPIOC_BASE, 4> vregEn;
typedef Gpio<GPIOG_BASE, 3> reset;
typedef Gpio<GPIOG_BASE, 9> cs;
typedef Gpio<GPIOG_BASE,13> sck;
typedef Gpio<GPIOG_BASE,14> mosi;
typedef Gpio<GPIOG_BASE, 7> dotclk;
typedef Gpio<GPIOC_BASE, 6> hsync;
typedef Gpio<GPIOA_BASE, 4> vsync;
typedef Gpio<GPIOF_BASE,10> en;
typedef Gpio<GPIOG_BASE, 6> r5;
typedef Gpio<GPIOB_BASE, 1> r4;
typedef Gpio<GPIOA_BASE,12> r3;
typedef Gpio<GPIOA_BASE,11> r2;
typedef Gpio<GPIOB_BASE, 0> r1;
typedef Gpio<GPIOC_BASE,10> r0;
typedef Gpio<GPIOD_BASE, 3> g5;
typedef Gpio<GPIOC_BASE, 7> g4;
typedef Gpio<GPIOB_BASE,11> g3;
typedef Gpio<GPIOB_BASE,10> g2;
typedef Gpio<GPIOG_BASE,10> g1;
typedef Gpio<GPIOA_BASE, 6> g0;
typedef Gpio<GPIOB_BASE, 9> b5;
typedef Gpio<GPIOB_BASE, 6> b4;
typedef Gpio<GPIOA_BASE, 3> b3;
typedef Gpio<GPIOG_BASE,12> b2;
typedef Gpio<GPIOG_BASE,11> b1;
typedef Gpio<GPIOD_BASE, 6> b0;
}

namespace unused {
typedef Gpio<GPIOA_BASE, 8> u1;
typedef Gpio<GPIOB_BASE, 3> u2;
typedef Gpio<GPIOB_BASE, 7> u3;
typedef Gpio<GPIOC_BASE, 5> u4;
typedef Gpio<GPIOC_BASE, 9> u5;
typedef Gpio<GPIOC_BASE,13> u6;
typedef Gpio<GPIOC_BASE,14> u7;
typedef Gpio<GPIOC_BASE,15> u8;
typedef Gpio<GPIOD_BASE, 4> u9;
typedef Gpio<GPIOD_BASE, 5> u10;
typedef Gpio<GPIOD_BASE,11> u11;
typedef Gpio<GPIOD_BASE,12> u12;
typedef Gpio<GPIOD_BASE,13> u13;
typedef Gpio<GPIOE_BASE, 6> u14;
typedef Gpio<GPIOF_BASE, 6> u15;
typedef Gpio<GPIOF_BASE, 7> u16;
typedef Gpio<GPIOF_BASE, 8> u17;
typedef Gpio<GPIOF_BASE, 9> u18;
typedef Gpio<GPIOG_BASE, 2> u19;
}

} //namespace miosix

#endif //HWMAPPING_H
