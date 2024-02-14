/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
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

typedef Gpio<GPIOA_BASE,0>  strig;
typedef Gpio<GPIOA_BASE,2>  button;
typedef Gpio<GPIOA_BASE,13> led;
typedef Gpio<GPIOA_BASE,14> hpled;
typedef Gpio<GPIOA_BASE,15> sen;

namespace nrf {
typedef Gpio<GPIOA_BASE,1>  irq;
typedef Gpio<GPIOA_BASE,4>  cs;
typedef Gpio<GPIOA_BASE,5>  sck;
typedef Gpio<GPIOA_BASE,6>  miso;
typedef Gpio<GPIOA_BASE,7>  mosi;
typedef Gpio<GPIOA_BASE,8>  ce;
}

namespace cam {
typedef Gpio<GPIOA_BASE,3>  irq;
typedef Gpio<GPIOB_BASE,11> en;
typedef Gpio<GPIOB_BASE,12> cs;
typedef Gpio<GPIOB_BASE,13> sck;
typedef Gpio<GPIOB_BASE,14> miso;
typedef Gpio<GPIOB_BASE,15> mosi;
}

namespace serial {
typedef Gpio<GPIOA_BASE,9>  tx;
typedef Gpio<GPIOA_BASE,10> rx;
}

} //namespace miosix

#endif //HWMAPPING_H
