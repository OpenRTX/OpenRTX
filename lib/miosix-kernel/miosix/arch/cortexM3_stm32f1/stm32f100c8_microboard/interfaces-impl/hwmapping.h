/***************************************************************************
 *   Copyright (C) 2017 by Terraneo Federico                               *
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

typedef Gpio<GPIOB_BASE,9> redLed;
typedef Gpio<GPIOB_BASE,7> yellowLed;

namespace expansion {
typedef Gpio<GPIOA_BASE, 0> io0;
typedef Gpio<GPIOA_BASE, 1> io1;
typedef Gpio<GPIOA_BASE, 2> io2;
typedef Gpio<GPIOA_BASE, 3> io3;
typedef Gpio<GPIOA_BASE, 4> io4;
typedef Gpio<GPIOA_BASE, 5> io5;
typedef Gpio<GPIOA_BASE, 6> io6;
typedef Gpio<GPIOA_BASE, 7> io7;
typedef Gpio<GPIOB_BASE, 0> io8;
typedef Gpio<GPIOB_BASE, 1> io9;
typedef Gpio<GPIOB_BASE,10> io10;
typedef Gpio<GPIOB_BASE,11> io11;
}

} //namespace miosix
