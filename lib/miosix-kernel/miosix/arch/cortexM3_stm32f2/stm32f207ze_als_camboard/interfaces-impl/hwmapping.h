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
typedef Gpio<GPIOD_BASE,13> a18;
}

//640x480 camera
namespace camera {
typedef Gpio<GPIOC_BASE,6>  cd0;
typedef Gpio<GPIOC_BASE,7>  cd1;
typedef Gpio<GPIOC_BASE,8>  cd2;
typedef Gpio<GPIOC_BASE,9>  cd3;
typedef Gpio<GPIOE_BASE,4>  cd4;
typedef Gpio<GPIOB_BASE,6>  cd5;
typedef Gpio<GPIOE_BASE,5>  cd6;
typedef Gpio<GPIOE_BASE,6>  cd7;
typedef Gpio<GPIOB_BASE,7>  vsync;
typedef Gpio<GPIOA_BASE,4>  hsync;
typedef Gpio<GPIOA_BASE,6>  dclk;
typedef Gpio<GPIOA_BASE,8>  exclk;
typedef Gpio<GPIOC_BASE,0>  reset;
typedef Gpio<GPIOC_BASE,1>  sda;
typedef Gpio<GPIOC_BASE,2>  scl;
}

//2MBytes SPI flash
namespace flash {
typedef Gpio<GPIOB_BASE,3>  sck;
typedef Gpio<GPIOB_BASE,4>  miso;
typedef Gpio<GPIOB_BASE,5>  mosi;
typedef Gpio<GPIOB_BASE,8>  cs;
}

//Board to board communication, SPI based
namespace comm {
typedef Gpio<GPIOC_BASE,10> sck;
typedef Gpio<GPIOC_BASE,11> miso;
typedef Gpio<GPIOC_BASE,12> mosi;
typedef Gpio<GPIOA_BASE,15> cs;
typedef Gpio<GPIOD_BASE,2>  irq;
}

//An unpopulated USB connection
namespace usb {
typedef Gpio<GPIOB_BASE,14> dm;
typedef Gpio<GPIOB_BASE,15> dp;
}

//Serial port
namespace serial {
typedef Gpio<GPIOA_BASE,9>  tx;
typedef Gpio<GPIOA_BASE,10> rx;
}

//Unused pins, configured as pulldown
namespace unused {
typedef Gpio<GPIOA_BASE,0>  u1;
typedef Gpio<GPIOA_BASE,1>  u2;
typedef Gpio<GPIOA_BASE,2>  u3;
typedef Gpio<GPIOA_BASE,3>  u4;
typedef Gpio<GPIOA_BASE,5>  u5;
typedef Gpio<GPIOA_BASE,7>  u6;
typedef Gpio<GPIOA_BASE,11> u7;
typedef Gpio<GPIOA_BASE,12> u8;
typedef Gpio<GPIOA_BASE,13> u9;  //Actually swdio
typedef Gpio<GPIOA_BASE,14> u10; //Actually swclk
typedef Gpio<GPIOC_BASE,13> u11; //Connected to comm::cs as a bug
typedef Gpio<GPIOB_BASE,0>  u12;
typedef Gpio<GPIOB_BASE,1>  u13;
typedef Gpio<GPIOB_BASE,2>  u14; //Connected to GND as it is BOOT1
typedef Gpio<GPIOB_BASE,9>  u15;
typedef Gpio<GPIOB_BASE,10> u16;
typedef Gpio<GPIOB_BASE,11> u17;
typedef Gpio<GPIOB_BASE,12> u18;
typedef Gpio<GPIOB_BASE,13> u19;
typedef Gpio<GPIOC_BASE,3>  u20;
typedef Gpio<GPIOC_BASE,4>  u21;
typedef Gpio<GPIOC_BASE,5>  u22;
typedef Gpio<GPIOC_BASE,14> u23;
typedef Gpio<GPIOC_BASE,15> u24;
typedef Gpio<GPIOD_BASE,3>  u25;
typedef Gpio<GPIOD_BASE,6>  u26; //Connected to comm::cs to simplify PCB routing
typedef Gpio<GPIOE_BASE,2>  u27;
typedef Gpio<GPIOE_BASE,3>  u28;
typedef Gpio<GPIOF_BASE,6>  u29;
typedef Gpio<GPIOF_BASE,7>  u30;
typedef Gpio<GPIOF_BASE,8>  u31;
typedef Gpio<GPIOF_BASE,9>  u32;
typedef Gpio<GPIOF_BASE,10> u33;
typedef Gpio<GPIOF_BASE,11> u34;
typedef Gpio<GPIOG_BASE,6>  u35;
typedef Gpio<GPIOG_BASE,7>  u36;
typedef Gpio<GPIOG_BASE,8>  u37;
typedef Gpio<GPIOG_BASE,9>  u38;
typedef Gpio<GPIOG_BASE,10> u39;
typedef Gpio<GPIOG_BASE,11> u40;
typedef Gpio<GPIOG_BASE,12> u41;
typedef Gpio<GPIOG_BASE,13> u42;
typedef Gpio<GPIOG_BASE,14> u43;
typedef Gpio<GPIOG_BASE,15> u44;
}

} //namespace miosix

#endif //HWMAPPING_H
