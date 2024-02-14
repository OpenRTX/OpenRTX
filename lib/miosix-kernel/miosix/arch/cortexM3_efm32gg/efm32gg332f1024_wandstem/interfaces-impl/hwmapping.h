/***************************************************************************
 *   Copyright (C) 2015, 2016 by Terraneo Federico                         *
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
#include "board_settings.h"

//NOTE: WANDSTEM_HW_REV is now defined in board_settings.h

namespace miosix {

typedef Gpio<GPIOF_BASE,2>  redLed;
typedef Gpio<GPIOA_BASE,4>  greenLed;   //Also pin 20 of expansion connector

//Also connected to a pin of the expansion connector in some revisions
//rev 1.0 pin 19
//rev 1.1 pin 30
//rev 1.2 pin 30
//rev 1.3 no longer connected to the expansion connector
typedef Gpio<GPIOA_BASE,3>  userButton;

//This is used for the VHT implementation, allowing to resynchronize
//the high frequency timer with the RTC every time the node goes out
//of deep sleep giving the impression of having an uninterrupted
//high frequency clock. The 32KHz frequency is also output on pin 26
//of the expansion connector providing a low frequency clock to
//daughter boards
typedef Gpio<GPIOA_BASE,10> loopback32KHzIn;
typedef Gpio<GPIOD_BASE,8>  loopback32KHzOut;

#if WANDSTEM_HW_REV>12
//low  = 2.3V
//high = 3.1V
typedef Gpio<GPIOE_BASE,15> voltageSelect;
#endif

#if WANDSTEM_HW_REV>13
//Revision 1.4 separated the radio power management from expansion connector
//power management. Before revision 1.4, transceiver::vregEn served both
//functions, while starting from revision 1.4, this pin controls the power
//switch for the expansion connector, and transceiver::vregEn controls the
//cc2520 and flash power domain
typedef Gpio<GPIOC_BASE,10> powerSwitch;
#endif //rev 1.4 or higher

namespace expansion {
//The 30-pin expansion connector exposes some pins of the microcontroller
//that are freely usable as GPIO by daughter boards, and are named from gpio0
//to gpio19. Each GPIO can have up to two alternate functions.
//Revision 1.4 uses reserves two GPIOs for internal use, the former gpio3 and
//gpio17, reducing the GPIO count from 20 to 18.
//MCU pin                   GPIO#     CONN# AF1          AF2
#if WANDSTEM_HW_REV==10
typedef Gpio<GPIOD_BASE,4>  gpio0;  //    1 ADC_CH0
typedef Gpio<GPIOD_BASE,5>  gpio1;  //    2 ADC_CH1
typedef Gpio<GPIOD_BASE,6>  gpio2;  //    3 ADC_CH2      LETIMER0
#else //rev 1.1 or greater
typedef Gpio<GPIOD_BASE,3>  gpio0;  //    1 ADC_CH0
typedef Gpio<GPIOD_BASE,6>  gpio1;  //    2 ADC_CH1      LETIMER0
typedef Gpio<GPIOD_BASE,5>  gpio2;  //    3 ADC_CH2
#endif
#if WANDSTEM_HW_REV<14
typedef Gpio<GPIOD_BASE,7>  gpio3;  //    4 ADC_CH3      reserved in rev 1.4
#endif //rev 1.3 or lower
typedef Gpio<GPIOC_BASE,5>  gpio4;  //    7 SPI_CS       LETIMER1
typedef Gpio<GPIOC_BASE,4>  gpio5;  //    8 SPI_SCK
typedef Gpio<GPIOC_BASE,3>  gpio6;  //    9 SPI_MISO     USART_RX
typedef Gpio<GPIOC_BASE,2>  gpio7;  //   10 SPI_MOSI     USART_TX
typedef Gpio<GPIOC_BASE,6>  gpio8;  //   11 I2C_SDA      LEUSART_TX
typedef Gpio<GPIOC_BASE,7>  gpio9;  //   12 I2C_SCL      LEUSART_RX
typedef Gpio<GPIOE_BASE,12> gpio10; //   13 TIMESTAMP_IN/OUT
typedef Gpio<GPIOB_BASE,11> gpio11; //   14 DAC_OUT
typedef Gpio<GPIOA_BASE,0>  gpio12; //   15 PWM0         PRS0
typedef Gpio<GPIOA_BASE,1>  gpio13; //   16 PWM1         PRS1
typedef Gpio<GPIOC_BASE,1>  gpio14; //   18              EXC_ACMP1
typedef Gpio<GPIOC_BASE,8>  gpio15; //   23 ACMP0
typedef Gpio<GPIOC_BASE,9>  gpio16; //   24 ACMP1
#if WANDSTEM_HW_REV<14
typedef Gpio<GPIOC_BASE,10> gpio17; //   25 ACMP2        reserved in rev 1.4
#endif //rev 1.3 or lower
typedef Gpio<GPIOE_BASE,8>  gpio18; //   27 PCNT_A
typedef Gpio<GPIOE_BASE,9>  gpio19; //   28 PCNT_B
} //namespace expansion

namespace internalSpi {
//The internal SPI is shared between the radio transceiver (CC2520) and flash
//(IS25LP128). In addition, the CC2520 can be configured to output an analog
//value proportional to its temperature on a pin that is shared with sck
typedef Gpio<GPIOD_BASE,0>  mosi;
typedef Gpio<GPIOD_BASE,1>  miso;
typedef Gpio<GPIOD_BASE,2>  sck;    //Also cc2520_tempsensor (analog)
} //namespace internalSpi

namespace transceiver {
//The radio transceiver. The exception channel B and STXON are connected to
//a timer input capture and output compare channel for precise packet timing
typedef Gpio<GPIOA_BASE,2>  cs;
typedef Gpio<GPIOF_BASE,5>  reset;
typedef Gpio<GPIOF_BASE,12> vregEn; //Also power switch enable before rev 1.4
typedef Gpio<GPIOE_BASE,13> gpio1;
typedef Gpio<GPIOE_BASE,14> gpio2;
typedef Gpio<GPIOA_BASE,8>  excChB; //including SFD and FRM_DONE
#if WANDSTEM_HW_REV<13
typedef Gpio<GPIOE_BASE,15> gpio4;
#endif
typedef Gpio<GPIOA_BASE,9>  stxon;
} //namespace transceiver

namespace flash {
//The on-board flash is a 16MByte IS25LP128, works down to 2.3V
typedef Gpio<GPIOC_BASE,11> cs;
typedef Gpio<GPIOA_BASE,5>  hold;
} //namespace flash

namespace currentSense {
//The current sensor uses a MAX44284F and 0.12ohm shunt resistor.
//Using the internal 1.25V reference for the ADC, the measurement range is 208mA
//and the resolution is ~51uA. The current sensor can sense the consumption of
//all the components on the board (MCU, transceiver, flash) and also of the
//components on the daughter board, unless they are hooked up to the VBAT line.
typedef Gpio<GPIOC_BASE,0>  enable;
#if WANDSTEM_HW_REV==10
typedef Gpio<GPIOD_BASE,3>  sense;  //Analog, also pin 5 of expansion connector
#else //rev 1.1 or greater
typedef Gpio<GPIOD_BASE,4>  sense;  //Analog, also pin 5 of expansion connector
#endif
} //namespace currentSense

//Rev 1.4 introduced a sensor for the battery voltage. This is done using a
//voltage divider that is enabled when the cc2520 voltage domain is enabled.
//The voltage that can be sensed at this point is the battery voltage
//multiplied by 0.237
#if WANDSTEM_HW_REV>13
typedef Gpio<GPIOD_BASE,7>  voltageSense;
#endif //rev 1.4 or higher

namespace debugConnector {
//The debug connector exposes a serial port for printf/scanf debugging, and
//the SWD debug interface. The connector is also used to start the bootloader
//to upload code to the board, by pulling SWCLK high and resetting the board.
//The bootloader can load code either using the serial port or the USB port.
//Finally, also the MCU reset is exposed.
typedef Gpio<GPIOE_BASE,10> tx;     //kernel serial port
typedef Gpio<GPIOE_BASE,11> rx;     //kernel serial port
typedef Gpio<GPIOF_BASE,0>  swclk;  //SWD (also pull high to start bootloader)
typedef Gpio<GPIOF_BASE,1>  swdio;  //SWD
} //namespace debugConnector

namespace usb {
//USB lines
typedef Gpio<GPIOF_BASE,10> dm;
typedef Gpio<GPIOF_BASE,11> dp;
} //namespace usb

} //namespace miosix

#endif //HWMAPPING_H
