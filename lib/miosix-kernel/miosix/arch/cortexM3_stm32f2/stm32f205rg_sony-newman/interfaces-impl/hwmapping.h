/***************************************************************************
 *   Copyright (C) 2013 by Terraneo Federico                               *
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

/*
 * The pin names were taken from underverk's SmartWatch_Toolchain/src/system.c
 * They were put in a comment saying "Sony's name". Probably the one who wrote
 * that file got access to much more documentation that the one that's publicly
 * available.
 */

// The OLED display is an LD7131 and has its own dedicated SPI bus
namespace oled {
typedef Gpio<GPIOA_BASE, 4> OLED_nSS_Pin;  //Sony calls it SPI1_nSS_Pin
typedef Gpio<GPIOA_BASE, 5> OLED_SCK_Pin;  //Sony calls it SPI1_SCK_Pin
typedef Gpio<GPIOA_BASE, 7> OLED_MOSI_Pin; //Sony calls it SPI1_MOSI_Pin
typedef Gpio<GPIOC_BASE, 0> OLED_A0_Pin;
typedef Gpio<GPIOB_BASE, 5> OLED_Reset_Pin;
typedef Gpio<GPIOC_BASE, 1> OLED_V_ENABLE_Pin; //Enables OLED panel 16V supply
}

// The touch controller is a CY8C20236 is connected to the I2C bus, address 0x0a
namespace touch {
typedef Gpio<GPIOB_BASE, 0> Touch_Reset_Pin;
typedef Gpio<GPIOC_BASE, 6> TOUCH_WKUP_INT_Pin;
}

// There is a PMU chip, unknown part, connected to the I2C bus, address 0x90
namespace power {
typedef Gpio<GPIOA_BASE, 1> BATT_V_ON_Pin; //Enables battery voltage sensor
typedef Gpio<GPIOA_BASE, 2> BAT_V_Pin;     //Analog input
typedef Gpio<GPIOB_BASE, 4> ENABLE_LIGHT_SENSOR_Pin;     //Enables light sensor
typedef Gpio<GPIOC_BASE, 4> LIGHT_SENSOR_ANALOG_OUT_Pin; //Analog input
typedef Gpio<GPIOB_BASE, 3> ENABLE_2V8_Pin; //Is in some way releted to the OLED
//Looks connected to the PMU, the most likely scenario is this: asserting it low
//when the USB is not connected disables the PMU voltage regulator feeding the
//microcontroller, therefore shutting down the watch.
typedef Gpio<GPIOC_BASE, 3> HoldPower_Pin;
}

// The accelerometer is an LIS3DH, connected to the I2C bus, address 0x30
typedef Gpio<GPIOB_BASE, 9> ACCELEROMETER_INT_Pin;

// The Touch controller, PMU and accelerometer are connected to the I2C bus
namespace i2c {
typedef Gpio<GPIOB_BASE, 6> I2C_SCL_Pin;
typedef Gpio<GPIOB_BASE, 7> I2C_SDA_Pin;
}

// Vibrator motor
typedef Gpio<GPIOB_BASE, 8> BUZER_PWM_Pin;

// The power button. Someone on stackoverflow mantions that if you push it for
// 10 seconds when the USB cable is disconnected, the watch shutdowns no matter
// what the software does (so, even if it is locked up). This means it's
// probably connected to the PMU as well.
typedef Gpio<GPIOB_BASE,11> POWER_BTN_PRESS_Pin; //Goes high when pressed FIXME: check

// USB connections
namespace usb {
typedef Gpio<GPIOA_BASE, 9> USB5V_Detected_Pin; //Goes high when USB connected FIXME: check
typedef Gpio<GPIOA_BASE,11> USB_DM_Pin;
typedef Gpio<GPIOA_BASE,12> USB_DP_Pin;
}

// Other than that it's an STLC2690, little is known about the bluetooth chip
namespace bluetooth {
//FIXME: which one is the right one?
typedef Gpio<GPIOA_BASE, 6> Reset_BT2_Pin; //According to sony's website
typedef Gpio<GPIOA_BASE,15> Reset_BT_Pin;  //According to underverk's SmartWatch_Toolchain

typedef Gpio<GPIOB_BASE,10> BT_CLK_REQ_Pin;
typedef Gpio<GPIOC_BASE,13> HOST_WAKE_UP_Pin;
typedef Gpio<GPIOC_BASE, 7> Enable_1V8_BT_Power_Pin;
typedef Gpio<GPIOC_BASE, 9> BT_nSS_Pin;  //Sony calls it SPI3_nSS_Pin
typedef Gpio<GPIOC_BASE,10> BT_SCK_Pin;  //Sony calls it SPI3_SCK_Pin
typedef Gpio<GPIOC_BASE,11> BT_MISO_Pin; //Sony calls it SPI3_MISO_Pin
typedef Gpio<GPIOC_BASE,12> BT_MOSI_Pin; //Sony calls it SPI3_MOSI_Pin
}

// The mistery pins mentioned in system.c, but never used
namespace unknown {
typedef Gpio<GPIOA_BASE, 0> WKUP_Pin;
//Stands for main clock out, a feature of the STM32 to output an internal clock
//(either the crystal one, or the PLL one). Ususally is used to clock some
//other chip saving a clock crystal in the BOM. Maybe it goes to the touchscreen
//controller? Who knows...
typedef Gpio<GPIOA_BASE, 8> MCO1_Pin;
typedef Gpio<GPIOB_BASE, 1> Connect_USB_Pin;
typedef Gpio<GPIOB_BASE, 2> POWER_3V3_ON_1V8_OFF_Pin;
typedef Gpio<GPIOB_BASE,12> SPI2_nSS_Pin; //Is there yet another mistery chip
typedef Gpio<GPIOB_BASE,13> SPI2_SCK_Pin; //connected to this SPI? I don't know
typedef Gpio<GPIOB_BASE,14> SPI2_MISO_Pin;
typedef Gpio<GPIOB_BASE,15> SPI2_MOSI_Pin;
}

} //namespace miosix

#endif //HWMAPPING_H
