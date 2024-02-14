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

typedef Gpio<GPIOE_BASE,2> led;

//Spare GPIOs, brought out to a connector
namespace gpio {
typedef Gpio<GPIOE_BASE,3> g0;
typedef Gpio<GPIOE_BASE,4> g1;
typedef Gpio<GPIOE_BASE,5> g2;
typedef Gpio<GPIOE_BASE,6> g3;
}

//Gpios that connect the ethernet transceiver to the microcontroller
namespace mii {
typedef Gpio<GPIOC_BASE,1>  mdc;
typedef Gpio<GPIOA_BASE,2>  mdio;
typedef Gpio<GPIOA_BASE,8>  clk;
typedef Gpio<GPIOC_BASE,6>  res;
typedef Gpio<GPIOC_BASE,0>  irq;
typedef Gpio<GPIOA_BASE,3>  col;
typedef Gpio<GPIOA_BASE,0>  crs;
typedef Gpio<GPIOA_BASE,1>  rxc;
typedef Gpio<GPIOA_BASE,7>  rxdv;
typedef Gpio<GPIOB_BASE,10> rxer;
typedef Gpio<GPIOC_BASE,4>  rxd0;
typedef Gpio<GPIOC_BASE,5>  rxd1;
typedef Gpio<GPIOB_BASE,0>  rxd2;
typedef Gpio<GPIOB_BASE,1>  rxd3;
typedef Gpio<GPIOB_BASE,11> txen;
typedef Gpio<GPIOC_BASE,3>  txc;
typedef Gpio<GPIOB_BASE,12> txd0;
typedef Gpio<GPIOB_BASE,13> txd1;
typedef Gpio<GPIOC_BASE,2>  txd2;
typedef Gpio<GPIOB_BASE,8>  txd3;
}

//External SRAM, connected to FSMC
namespace sram {
typedef Gpio<GPIOD_BASE,7>  cs1;
typedef Gpio<GPIOD_BASE,4>  oe;
typedef Gpio<GPIOD_BASE,5>  we;
typedef Gpio<GPIOE_BASE,0>  lb;
typedef Gpio<GPIOE_BASE,1>  ub;
typedef Gpio<GPIOD_BASE,14> d0;
typedef Gpio<GPIOD_BASE,15> d1;
typedef Gpio<GPIOD_BASE,0>  d2;
typedef Gpio<GPIOD_BASE,1>  d3;
typedef Gpio<GPIOE_BASE,7>  d4;
typedef Gpio<GPIOE_BASE,8>  d5;
typedef Gpio<GPIOE_BASE,9>  d6;
typedef Gpio<GPIOE_BASE,10> d7;
typedef Gpio<GPIOE_BASE,11> d8;
typedef Gpio<GPIOE_BASE,12> d9;
typedef Gpio<GPIOE_BASE,13> d10;
typedef Gpio<GPIOE_BASE,14> d11;
typedef Gpio<GPIOE_BASE,15> d12;
typedef Gpio<GPIOD_BASE,8>  d13;
typedef Gpio<GPIOD_BASE,9>  d14;
typedef Gpio<GPIOD_BASE,10> d15;
typedef Gpio<GPIOF_BASE,0>  a0;
typedef Gpio<GPIOF_BASE,1>  a1;
typedef Gpio<GPIOF_BASE,2>  a2;
typedef Gpio<GPIOF_BASE,3>  a3;
typedef Gpio<GPIOF_BASE,4>  a4;
typedef Gpio<GPIOF_BASE,5>  a5;
typedef Gpio<GPIOF_BASE,12> a6;
typedef Gpio<GPIOF_BASE,13> a7;
typedef Gpio<GPIOF_BASE,14> a8;
typedef Gpio<GPIOF_BASE,15> a9;
typedef Gpio<GPIOG_BASE,0>  a10;
typedef Gpio<GPIOG_BASE,1>  a11;
typedef Gpio<GPIOG_BASE,2>  a12;
typedef Gpio<GPIOG_BASE,3>  a13;
typedef Gpio<GPIOG_BASE,4>  a14;
typedef Gpio<GPIOG_BASE,5>  a15;
typedef Gpio<GPIOD_BASE,11> a16;
typedef Gpio<GPIOD_BASE,12> a17;
}

//Connections to the optional nRF24L01 radio module
namespace nrf {
typedef Gpio<GPIOA_BASE,4> cs;
typedef Gpio<GPIOA_BASE,5> sck;
typedef Gpio<GPIOA_BASE,6> miso;
typedef Gpio<GPIOB_BASE,5> mosi;
typedef Gpio<GPIOF_BASE,6> ce;
typedef Gpio<GPIOF_BASE,7> irq;
}

//Serial port
namespace serial {
typedef Gpio<GPIOA_BASE,9>  tx;
typedef Gpio<GPIOA_BASE,10> rx;
}

//USB host port
namespace usbhost {
typedef Gpio<GPIOA_BASE,11> dm;
typedef Gpio<GPIOA_BASE,12> dp;
}

//USB device port
namespace usbdevice {
typedef Gpio<GPIOB_BASE,14> dm;
typedef Gpio<GPIOB_BASE,15> dp;
}

//MicroSD card slot
namespace sdio {
typedef Gpio<GPIOC_BASE,8>  d0;
typedef Gpio<GPIOC_BASE,9>  d1;
typedef Gpio<GPIOC_BASE,10> d2;
typedef Gpio<GPIOC_BASE,11> d3;
typedef Gpio<GPIOC_BASE,12> ck;
typedef Gpio<GPIOC_BASE,13> cd;
typedef Gpio<GPIOD_BASE,2>  cmd;
}

//JTAG port
namespace jtag {
typedef Gpio<GPIOA_BASE,15> tdi;
typedef Gpio<GPIOB_BASE,3>  tdo;
typedef Gpio<GPIOA_BASE,13> tms;
typedef Gpio<GPIOA_BASE,14> tck;
typedef Gpio<GPIOB_BASE,4>  trst;
}

//Unused pins, configured as pulldown
namespace unused {
typedef Gpio<GPIOB_BASE,2>  u1;  //Connected to ground, as it is boot1
typedef Gpio<GPIOB_BASE,6>  u2;
typedef Gpio<GPIOB_BASE,7>  u3;
typedef Gpio<GPIOB_BASE,9>  u4;
typedef Gpio<GPIOC_BASE,7>  u5;
typedef Gpio<GPIOC_BASE,14> u6;
typedef Gpio<GPIOC_BASE,15> u7;
typedef Gpio<GPIOD_BASE,3>  u8;
typedef Gpio<GPIOD_BASE,6>  u9;
typedef Gpio<GPIOD_BASE,13> u10;
typedef Gpio<GPIOF_BASE,8>  u11;
typedef Gpio<GPIOF_BASE,9>  u12;
typedef Gpio<GPIOF_BASE,10> u13;
typedef Gpio<GPIOF_BASE,11> u14;
typedef Gpio<GPIOG_BASE,6>  u15;
typedef Gpio<GPIOG_BASE,7>  u16;
typedef Gpio<GPIOG_BASE,8>  u17;
typedef Gpio<GPIOG_BASE,9>  u18;
typedef Gpio<GPIOG_BASE,10> u19;
typedef Gpio<GPIOG_BASE,11> u20;
typedef Gpio<GPIOG_BASE,12> u21;
typedef Gpio<GPIOG_BASE,13> u22;
typedef Gpio<GPIOG_BASE,14> u23;
typedef Gpio<GPIOG_BASE,15> u24;
}

} //namespace miosix

#endif //HWMAPPING_H
