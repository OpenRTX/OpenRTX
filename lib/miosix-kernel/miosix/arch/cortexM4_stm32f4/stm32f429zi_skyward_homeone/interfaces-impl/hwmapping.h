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

#ifndef HWMAPPING_H
#define	HWMAPPING_H

#include "interfaces/gpio.h"

namespace miosix {

namespace interfaces {

namespace spi1 {
using sck   = Gpio<GPIOA_BASE, 5>;
using miso  = Gpio<GPIOA_BASE, 6>;
using mosi  = Gpio<GPIOA_BASE, 7>;
} //namespace spi1

namespace spi2 {
using sck   = Gpio<GPIOB_BASE, 13>;
using miso  = Gpio<GPIOB_BASE, 14>;
using mosi  = Gpio<GPIOB_BASE, 15>;
} //namespace spi1

namespace i2c {
using scl   = Gpio<GPIOB_BASE, 8>;
using sda   = Gpio<GPIOB_BASE, 9>;
} //namespace i2c

namespace uart4 {
using tx    = Gpio<GPIOA_BASE, 0>;
using rx    = Gpio<GPIOA_BASE, 1>;
} //namespace uart4

namespace can {
using rx    = Gpio<GPIOA_BASE, 11>;
using tx    = Gpio<GPIOA_BASE, 12>;
} // namespace can
} //namespace interfaces

namespace sensors {
    
namespace adis16405 {
using cs    = Gpio<GPIOA_BASE, 8>;
using dio1  = Gpio<GPIOB_BASE, 4>;
using nrst  = Gpio<GPIOD_BASE, 5>;
using ckIn  = Gpio<GPIOD_BASE, 14>;
} //namespace adis16405

namespace ad7994 {
using ab      = Gpio<GPIOB_BASE, 1>;
using nconvst = Gpio<GPIOG_BASE, 9>;
} //namespace ad7994

namespace max21105 {
using cs    = Gpio<GPIOC_BASE, 1>;
} //namespace max21105

namespace mpu9250 {
using cs    = Gpio<GPIOC_BASE, 3>;
} //namespace mpu9250

namespace ms5803 {
using cs    = Gpio<GPIOD_BASE, 7>;
} //namespace ms5803
} //namespace sensors

namespace actuators {
namespace hbridgel {
using ena   = Gpio<GPIOD_BASE, 11>;
using in    = Gpio<GPIOD_BASE, 13>;
using csens = Gpio<GPIOF_BASE, 8>;
} //namespace hbridgel

namespace hbridger {
using ena   = Gpio<GPIOD_BASE, 9>;
using in    = Gpio<GPIOD_BASE, 12>;
using csens = Gpio<GPIOF_BASE, 6>;
} //namespace hbridger
} //namespace actuators

namespace InAir9B {
using dio0  = Gpio<GPIOB_BASE, 0>;
using dio1  = Gpio<GPIOC_BASE, 4>;
using dio2  = Gpio<GPIOA_BASE, 4>;
using dio3  = Gpio<GPIOC_BASE, 2>;
using cs    = Gpio<GPIOF_BASE, 9>;
using nrst  = Gpio<GPIOF_BASE, 7>;
} //namespace InAir9B
} //namespace miosix

#endif //HWMAPPING_H
