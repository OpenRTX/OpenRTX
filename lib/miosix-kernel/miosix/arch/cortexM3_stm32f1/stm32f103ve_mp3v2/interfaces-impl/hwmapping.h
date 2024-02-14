
/***************************************************************************
 *   Copyright (C) 2011 by Terraneo Federico                               *
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
 ***************************************************************************
 *
 * *****************
 * Version 1.01 beta
 * 01/03/2011
 * *****************
 */

#ifndef HWMAPPING_H
#define HWMAPPING_H

#ifdef _MIOSIX
#include "interfaces/gpio.h"
namespace miosix {
#else //_MIOSIX
#include "gpio.h"

//These two functions are only available from the bootloader
/**
 * Enable +3v3b and +1v8 domains, etc...
 */
void powerOn();

/**
 * Disable +3v3b and +1v8 domains, etc...
 */
void powerOff();

#endif //_MIOSIX

//
// All GPIOs are mapped here
//

//Buttons
typedef Gpio<GPIOA_BASE,0>  button1; //Active low, generates irq for boot
typedef Gpio<GPIOA_BASE,1>  button2; //Active low

//LED
typedef Gpio<GPIOB_BASE,12> led; //Active high

//Display interface
namespace disp {
typedef Gpio<GPIOB_BASE,0>  yp; //Touchscreen Y+ (analog)
typedef Gpio<GPIOB_BASE,1>  ym; //Touchscreen Y- (analog)
typedef Gpio<GPIOC_BASE,3>  xp; //Touchscreen X+ (analog, with 330ohm in series)
typedef Gpio<GPIOC_BASE,4>  xm; //Touchscreen X- (analog)
typedef Gpio<GPIOD_BASE,3>  ncpEn; //Active high
typedef Gpio<GPIOD_BASE,14> d0;  //Handled by hardware (FSMC)
typedef Gpio<GPIOD_BASE,15> d1;  //Handled by hardware (FSMC)
typedef Gpio<GPIOD_BASE,0>  d2;  //Handled by hardware (FSMC)
typedef Gpio<GPIOD_BASE,1>  d3;  //Handled by hardware (FSMC)
typedef Gpio<GPIOE_BASE,7>  d4;  //Handled by hardware (FSMC)
typedef Gpio<GPIOE_BASE,8>  d5;  //Handled by hardware (FSMC)
typedef Gpio<GPIOE_BASE,9>  d6;  //Handled by hardware (FSMC)
typedef Gpio<GPIOE_BASE,10> d7;  //Handled by hardware (FSMC)
typedef Gpio<GPIOE_BASE,11> d8;  //Handled by hardware (FSMC)
typedef Gpio<GPIOE_BASE,12> d9;  //Handled by hardware (FSMC)
typedef Gpio<GPIOE_BASE,13> d10; //Handled by hardware (FSMC)
typedef Gpio<GPIOE_BASE,14> d11; //Handled by hardware (FSMC)
typedef Gpio<GPIOE_BASE,15> d12; //Handled by hardware (FSMC)
typedef Gpio<GPIOD_BASE,8>  d13; //Handled by hardware (FSMC)
typedef Gpio<GPIOD_BASE,9>  d14; //Handled by hardware (FSMC)
typedef Gpio<GPIOD_BASE,10> d15; //Handled by hardware (FSMC)
typedef Gpio<GPIOD_BASE,11> rs;  //Handled by hardware (FSMC)
typedef Gpio<GPIOD_BASE,4>  rd;  //Handled by hardware (FSMC)
typedef Gpio<GPIOD_BASE,5>  wr;  //Handled by hardware (FSMC)
typedef Gpio<GPIOD_BASE,7>  cs;  //Handled by hardware (FSMC), 100k to +3v3a
typedef Gpio<GPIOD_BASE,6>  reset; //Active low
}

//Audio DSP connections
namespace dsp {
typedef Gpio<GPIOA_BASE,4>  xcs;
typedef Gpio<GPIOA_BASE,5>  sclk;   //Handled by hardware (SPI1)
typedef Gpio<GPIOA_BASE,6>  so;     //Handled by hardware (SPI1)
typedef Gpio<GPIOA_BASE,7>  si;     //Handled by hardware (SPI1)
typedef Gpio<GPIOB_BASE,5>  xreset;
typedef Gpio<GPIOE_BASE,2>  dreq;   //Generates irq ?
typedef Gpio<GPIOE_BASE,3>  xdcs;
}

//MicroSD connections
namespace sd {
typedef Gpio<GPIOA_BASE,8>  cardDetect;
typedef Gpio<GPIOC_BASE,8>  d0;  //Handled by hardware (SDIO)
typedef Gpio<GPIOC_BASE,9>  d1;  //Handled by hardware (SDIO)
typedef Gpio<GPIOC_BASE,10> d2;  //Handled by hardware (SDIO)
typedef Gpio<GPIOC_BASE,11> d3;  //Handled by hardware (SDIO)
typedef Gpio<GPIOC_BASE,12> clk; //Handled by hardware (SDIO)
typedef Gpio<GPIOD_BASE,2>  cmd; //Handled by hardware (SDIO), 100k to +3v3b
}

//USB connections
namespace usb {
typedef Gpio<GPIOA_BASE,11> dm;     //Handled by hardware (USB) D-
typedef Gpio<GPIOA_BASE,12> dp;     //Handled by hardware (USB) D+
typedef Gpio<GPIOE_BASE,4>  detect; //1K5 pullup connected to D+
}

//USB Battery charger
namespace charger {
typedef Gpio<GPIOB_BASE,7>  done;
typedef Gpio<GPIOB_BASE,9>  seli; //100k to ground
}

//Power management
namespace pwrmgmt {
typedef Gpio<GPIOC_BASE,6>  vcc3v3bEn; //100k to +3v3a (active low)
typedef Gpio<GPIOC_BASE,7>  vcc1v8En;  //100k to ground
typedef Gpio<GPIOE_BASE,5>  vbatEn;    //If low allows to measure vbat
typedef Gpio<GPIOC_BASE,5>  vbat;      //Analog in to measure vbat
}

//Externam FLASH
namespace xflash {
typedef Gpio<GPIOB_BASE,6>  cs;  //100k to +3v3b
typedef Gpio<GPIOB_BASE,13> sck; //Handled by hardware (SPI2)
typedef Gpio<GPIOB_BASE,14> so;  //Handled by hardware (SPI2)
typedef Gpio<GPIOB_BASE,15> si;  //Handled by hardware (SPI2)
}

//Accelerometer
namespace accel {
typedef Gpio<GPIOC_BASE,0>  x; //Analog
typedef Gpio<GPIOC_BASE,1>  y; //Analog
typedef Gpio<GPIOC_BASE,2>  z; //Analog
}

//Debug/bootloader serial port
namespace boot {
typedef Gpio<GPIOB_BASE,2>  detect; //BOOT1 (10k to ground)
typedef Gpio<GPIOA_BASE,9>  tx;     //Handled by hardware (USART1)
typedef Gpio<GPIOA_BASE,10> rx;     //Handled by hardware (USART1)
}

//Expansion
namespace expansion {
typedef Gpio<GPIOE_BASE,0>  exp0;
typedef Gpio<GPIOE_BASE,1>  exp1;
typedef Gpio<GPIOA_BASE,2>  tx2;
typedef Gpio<GPIOA_BASE,3>  rx2;
typedef Gpio<GPIOB_BASE,10> tx3;
typedef Gpio<GPIOB_BASE,11> rx3;
}

//Unused pins
typedef Gpio<GPIOA_BASE,13> pa13;
typedef Gpio<GPIOA_BASE,14> pa14;
typedef Gpio<GPIOA_BASE,15> pa15;
typedef Gpio<GPIOB_BASE,3>  pb3;
typedef Gpio<GPIOB_BASE,4>  pb4;
typedef Gpio<GPIOC_BASE,13> pc13;
typedef Gpio<GPIOC_BASE,14> pc14; //This is actually for the 32KHz xtal
typedef Gpio<GPIOC_BASE,15> pc15; //This is actually for the 32KHz xtal
typedef Gpio<GPIOD_BASE,12> pd12;
typedef Gpio<GPIOD_BASE,13> pd13;
typedef Gpio<GPIOE_BASE,6>  pe6;
typedef Gpio<GPIOB_BASE,8>  pb8; //used to be Charger::en

#ifdef _MIOSIX
} //namespace miosix
#endif //_MIOSIX

#endif //HWMAPPING_H
