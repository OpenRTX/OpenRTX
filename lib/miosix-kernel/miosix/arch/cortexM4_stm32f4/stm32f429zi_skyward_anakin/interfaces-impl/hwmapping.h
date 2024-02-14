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

#ifndef HWMAPPING_H
#define	HWMAPPING_H

#include "interfaces/gpio.h"

namespace miosix {

namespace leds {
using led0  = Gpio<GPIOB_BASE, 0>; //Green, mapped to ledOn()/ledOff()
using led1  = Gpio<GPIOA_BASE,15>; //Green
using led2  = Gpio<GPIOB_BASE, 4>; //Red
using led3  = Gpio<GPIOC_BASE, 4>; //Red
using led4  = Gpio<GPIOC_BASE, 5>; //Red
using led5  = Gpio<GPIOF_BASE, 6>; //Red
using led6  = Gpio<GPIOD_BASE, 3>; //Red
using led7  = Gpio<GPIOD_BASE, 4>; //Red
using led8  = Gpio<GPIOG_BASE, 2>; //Red
using led9  = Gpio<GPIOG_BASE, 3>; //Red
} //namespace leds

namespace sensors {

//Shared SPI bus among sensors
using sck   = Gpio<GPIOA_BASE, 5>;
using miso  = Gpio<GPIOA_BASE, 6>;
using mosi  = Gpio<GPIOA_BASE, 7>;

//Shared I2C bus among sensors
using sda   = Gpio<GPIOB_BASE, 7>;
using scl   = Gpio<GPIOB_BASE, 8>;

//Gyro, SPI, 2MHz SCK max
namespace fxas21002 {
using cs    = Gpio<GPIOG_BASE,10>;
using int1  = Gpio<GPIOD_BASE, 7>;
using int2  = Gpio<GPIOA_BASE, 8>;
} //namespace fxas21002

//Barometer, SPI, <FIXME>MHz SCK max
namespace lps331 {
using cs    = Gpio<GPIOE_BASE, 4>;
using int1  = Gpio<GPIOA_BASE, 2>;
using int2  = Gpio<GPIOA_BASE, 3>;    
} //namespace lps331

//IMU, SPI, 10MHz SCK max
namespace lsm9ds {
using csg   = Gpio<GPIOG_BASE, 9>;
using csm   = Gpio<GPIOG_BASE,11>;
using drdyg = Gpio<GPIOB_BASE,14>;
using int1g = Gpio<GPIOD_BASE,11>;
using int1m = Gpio<GPIOD_BASE,12>;
using int2m = Gpio<GPIOC_BASE,13>;
} //namespace lsm9ds

//IMU, SPI, 10MHz SCK max
namespace max21105 {
using cs    = Gpio<GPIOE_BASE, 2>;
using int1  = Gpio<GPIOE_BASE, 5>;
using int2  = Gpio<GPIOE_BASE, 6>;
} //namespace max21105

//Thermocouple, SPI, 5MHz SCK max
namespace max31856 {
using cs    = Gpio<GPIOB_BASE,11>;
using drdy  = Gpio<GPIOB_BASE, 9>;
using fault = Gpio<GPIOF_BASE,10>;
} //namespace max31856

//Barometer, I2C, 400KHz SCL max
namespace mpl3115 {
using int1  = Gpio<GPIOA_BASE, 1>;
using int2  = Gpio<GPIOA_BASE, 4>;
} //namespace mpl5115

//IMU, SPI, 1MHz SCK max
namespace mpu9250 {
using cs    = Gpio<GPIOD_BASE,13>;
using int1  = Gpio<GPIOB_BASE,15>;
} //namespace mpu9250

//Barometer, SPI, 20MHz SCK max
namespace ms5803 {
using cs    = Gpio<GPIOB_BASE,10>;
} //namespace ms5803

} //namespace sensors

namespace can {
using tx1   = Gpio<GPIOA_BASE,12>;
using rx1   = Gpio<GPIOA_BASE,11>;
using tx2   = Gpio<GPIOB_BASE,13>;
using rx2   = Gpio<GPIOB_BASE,12>;
} //namespace can

namespace eth {
using miso  = Gpio<GPIOF_BASE, 8>;
using mosi  = Gpio<GPIOF_BASE, 9>;
using sck   = Gpio<GPIOF_BASE, 7>;
using cs    = Gpio<GPIOE_BASE, 3>;
using int1  = Gpio<GPIOC_BASE, 1>;
} //namespace eth

namespace wireless {
//FIXME cs is missing
using miso  = Gpio<GPIOG_BASE,12>;
using mosi  = Gpio<GPIOG_BASE,14>;
using sck   = Gpio<GPIOG_BASE,13>;
} //namespace wireless

namespace batteryManager {
using currentSense = Gpio<GPIOB_BASE, 1>; //analog
using voltageSense = Gpio<GPIOC_BASE, 3>; //analog  
} //namespace batteryManager

namespace misc {
using wkup     = Gpio<GPIOA_BASE, 0>;
using adc1     = Gpio<GPIOC_BASE, 2>; //analog
using usart6tx = Gpio<GPIOC_BASE, 6>;
using usart6rx = Gpio<GPIOC_BASE, 7>;
using bootsel0 = Gpio<GPIOG_BASE, 6>;
using bootsel1 = Gpio<GPIOG_BASE, 7>;
} //namespace misc

namespace piksi {
using tx       = Gpio<GPIOD_BASE, 5>;
using rx       = Gpio<GPIOD_BASE, 6>;
} //namespace piksi

} //namespace miosix

#endif //HWMAPPING_H
