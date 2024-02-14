/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
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

#ifndef HWMAPPING_H
#define	HWMAPPING_H

#include "interfaces/gpio.h"

namespace miosix {

//LEDs
typedef Gpio<GPIOB_BASE,0> led1;
typedef Gpio<GPIOB_BASE,7> led2;
typedef Gpio<GPIOB_BASE,14> led3;

//User button
typedef Gpio<GPIOC_BASE,13> btn;

//Serial port 
namespace serial {
typedef Gpio<GPIOD_BASE,8> tx;
typedef Gpio<GPIOD_BASE,9> rx;
}

//MicroSD card slot exposed on ST Zio connector
namespace sdio {
typedef Gpio<GPIOC_BASE,8>  d0;
typedef Gpio<GPIOC_BASE,9>  d1;
typedef Gpio<GPIOC_BASE,10> d2;
typedef Gpio<GPIOC_BASE,11> d3;
typedef Gpio<GPIOC_BASE,12> ck;
typedef Gpio<GPIOD_BASE,2>  cmd;
}

//USB OTG A/B port
namespace usb {
typedef Gpio<GPIOA_BASE,9>  vbus;
typedef Gpio<GPIOA_BASE,10> id;
typedef Gpio<GPIOA_BASE,11> dm;
typedef Gpio<GPIOA_BASE,12> dp;
typedef Gpio<GPIOG_BASE,6>  on;
}

//Gpios that connect the ethernet transceiver to the microcontroller
namespace rmii {
typedef Gpio<GPIOG_BASE,11> txen;
typedef Gpio<GPIOG_BASE,13> txd0;
typedef Gpio<GPIOB_BASE,13> txd1;
typedef Gpio<GPIOC_BASE,4>  rxd0;
typedef Gpio<GPIOC_BASE,5>  rxd1;
typedef Gpio<GPIOA_BASE,7>  crsdv;
typedef Gpio<GPIOC_BASE,1>  mdc;
typedef Gpio<GPIOA_BASE,2>  mdio;
typedef Gpio<GPIOA_BASE,1>  refclk;
}

//32kHz clock pins used by the RTC
namespace rtc {
typedef Gpio<GPIOC_BASE,14> osc32in;
typedef Gpio<GPIOC_BASE,15> osc32out;
}

//SWD lines
namespace swd {
typedef Gpio<GPIOB_BASE,3>  swo;
typedef Gpio<GPIOA_BASE,13> tms;
typedef Gpio<GPIOA_BASE,14> tck;
}

//Free pins
namespace unused {
typedef Gpio<GPIOA_BASE,0>  pa0;
typedef Gpio<GPIOA_BASE,3>  pa3;
typedef Gpio<GPIOA_BASE,4>  pa4;
typedef Gpio<GPIOA_BASE,5>  pa5;
typedef Gpio<GPIOA_BASE,6>  pa6;
typedef Gpio<GPIOA_BASE,8>  pa8;
typedef Gpio<GPIOA_BASE,15> pa15;
typedef Gpio<GPIOB_BASE,1>  pb1;
typedef Gpio<GPIOB_BASE,2>  pb2; //BOOT1
typedef Gpio<GPIOB_BASE,4>  pb4;
typedef Gpio<GPIOB_BASE,5>  pb5;
typedef Gpio<GPIOB_BASE,6>  pb6;
typedef Gpio<GPIOB_BASE,8>  pb8;
typedef Gpio<GPIOB_BASE,9>  pb9;
typedef Gpio<GPIOB_BASE,10> pb10;
typedef Gpio<GPIOB_BASE,11> pb11;
typedef Gpio<GPIOB_BASE,12> pb12;
typedef Gpio<GPIOB_BASE,15> pb15;
typedef Gpio<GPIOC_BASE,0>  pc0;
typedef Gpio<GPIOC_BASE,2>  pc2;
typedef Gpio<GPIOC_BASE,3>  pc3;
typedef Gpio<GPIOC_BASE,6>  pc6;
typedef Gpio<GPIOC_BASE,7>  pc7;
typedef Gpio<GPIOD_BASE,0>  pd0;
typedef Gpio<GPIOD_BASE,1>  pd1;
typedef Gpio<GPIOD_BASE,3>  pd3;
typedef Gpio<GPIOD_BASE,4>  pd4;
typedef Gpio<GPIOD_BASE,5>  pd5;
typedef Gpio<GPIOD_BASE,6>  pd6;
typedef Gpio<GPIOD_BASE,7>  pd7;
typedef Gpio<GPIOD_BASE,10> pd10;
typedef Gpio<GPIOD_BASE,11> pd11;
typedef Gpio<GPIOD_BASE,12> pd12;
typedef Gpio<GPIOD_BASE,13> pd13;
typedef Gpio<GPIOD_BASE,14> pd14;
typedef Gpio<GPIOD_BASE,15> pd15;
typedef Gpio<GPIOE_BASE,0>  pe0;
typedef Gpio<GPIOE_BASE,1>  pe1;
typedef Gpio<GPIOE_BASE,2>  pe2;
typedef Gpio<GPIOE_BASE,3>  pe3;
typedef Gpio<GPIOE_BASE,4>  pe4;
typedef Gpio<GPIOE_BASE,5>  pe5;
typedef Gpio<GPIOE_BASE,6>  pe6;
typedef Gpio<GPIOE_BASE,7>  pe7;
typedef Gpio<GPIOE_BASE,8>  pe8;
typedef Gpio<GPIOE_BASE,9>  pe9;
typedef Gpio<GPIOE_BASE,10> pe10;
typedef Gpio<GPIOE_BASE,11> pe11;
typedef Gpio<GPIOE_BASE,12> pe12;
typedef Gpio<GPIOE_BASE,13> pe13;
typedef Gpio<GPIOE_BASE,14> pe14;
typedef Gpio<GPIOE_BASE,15> pe15;
typedef Gpio<GPIOF_BASE,0>  pf0; //PH0
typedef Gpio<GPIOF_BASE,1>  pf1; //PH1
typedef Gpio<GPIOF_BASE,2>  pf2;
typedef Gpio<GPIOF_BASE,3>  pf3;
typedef Gpio<GPIOF_BASE,4>  pf4;
typedef Gpio<GPIOF_BASE,5>  pf5;
typedef Gpio<GPIOF_BASE,6>  pf6;
typedef Gpio<GPIOF_BASE,7>  pf7;
typedef Gpio<GPIOF_BASE,8>  pf8;
typedef Gpio<GPIOF_BASE,9>  pf9;
typedef Gpio<GPIOF_BASE,10> pf10;
typedef Gpio<GPIOF_BASE,11> pf11;
typedef Gpio<GPIOF_BASE,12> pf12;
typedef Gpio<GPIOF_BASE,13> pf13;
typedef Gpio<GPIOF_BASE,14> pf14;
typedef Gpio<GPIOF_BASE,15> pf15;
typedef Gpio<GPIOG_BASE,0>  pg0;
typedef Gpio<GPIOG_BASE,1>  pg1;
typedef Gpio<GPIOG_BASE,2>  pg2;
typedef Gpio<GPIOG_BASE,3>  pg3;
typedef Gpio<GPIOG_BASE,4>  pg4;
typedef Gpio<GPIOG_BASE,5>  pg5;
typedef Gpio<GPIOG_BASE,7>  pg7;
typedef Gpio<GPIOG_BASE,8>  pg8;
typedef Gpio<GPIOG_BASE,9>  pg9;
typedef Gpio<GPIOG_BASE,10> pg10;
typedef Gpio<GPIOG_BASE,12> pg12;
typedef Gpio<GPIOG_BASE,14> pg14;
typedef Gpio<GPIOG_BASE,15> pg15;
typedef Gpio<GPIOH_BASE,0>  ph0;
typedef Gpio<GPIOH_BASE,1>  ph1;
typedef Gpio<GPIOH_BASE,2>  ph2;
typedef Gpio<GPIOH_BASE,3>  ph3;
typedef Gpio<GPIOH_BASE,4>  ph4;
typedef Gpio<GPIOH_BASE,5>  ph5;
typedef Gpio<GPIOH_BASE,6>  ph6;
typedef Gpio<GPIOH_BASE,7>  ph7;
typedef Gpio<GPIOH_BASE,8>  ph8;
typedef Gpio<GPIOH_BASE,9>  ph9;
typedef Gpio<GPIOH_BASE,10> ph10;
typedef Gpio<GPIOH_BASE,11> ph11;
typedef Gpio<GPIOH_BASE,12> ph12;
typedef Gpio<GPIOH_BASE,13> ph13;
typedef Gpio<GPIOH_BASE,14> ph14;
typedef Gpio<GPIOH_BASE,15> ph15;
}

} //namespace miosix

#endif //HWMAPPING_H
