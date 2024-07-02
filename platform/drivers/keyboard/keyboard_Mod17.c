/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <peripherals/gpio.h>
#include <interfaces/delays.h>
#include <interfaces/keyboard.h>
#include <interfaces/platform.h>
#include <hwconfig.h>
#include <i2c_stm32.h>
#include "cap1206.h"


static bool hmiConnected = false;


void kbd_init()
{
    const hwInfo_t *hwinfo = platform_getHwInfo();
    if(((hwinfo->flags & MOD17_FLAGS_HMI_PRESENT) != 0) &&
       ((hwinfo->hw_version >> 8) == MOD17_HMI_V10))
    {
        /* Bring up the I2C2 interface for the touch controller on HMI */
        gpio_setMode(HMI_SMCLK,  ALTERNATE_OD | ALTERNATE_FUNC(4));
        gpio_setMode(HMI_SMDATA, ALTERNATE_OD | ALTERNATE_FUNC(4));
        gpio_setOutputSpeed(HMI_SMCLK, HIGH);
        gpio_setOutputSpeed(HMI_SMDATA, HIGH);

        i2c_init(&i2c2, I2C_SPEED_100kHz);
        cap1206_init(&i2c2);

        hmiConnected = true;
    }
    else
    {
        gpio_setMode(ESC_SW,   INPUT);
        gpio_setMode(ENTER_SW, INPUT);
        gpio_setMode(LEFT_SW,  INPUT);
        gpio_setMode(RIGHT_SW, INPUT);
        gpio_setMode(UP_SW,    INPUT);
        gpio_setMode(DOWN_SW,  INPUT);
    }
}

void kbd_terminate()
{

}

keyboard_t kbd_getKeys()
{
    keyboard_t keys = 0;

    if(hmiConnected)
    {
        int resp = cap1206_readkeys(&i2c2);
        if(resp < 0)
            return 0;

        if(resp & 1)  keys |= KEY_LEFT;  // CS1
        if(resp & 2)  keys |= KEY_DOWN;  // CS2
        if(resp & 4)  keys |= KEY_RIGHT; // CS3
        if(resp & 8)  keys |= KEY_ENTER; // CS4
        if(resp & 16) keys |= KEY_UP;    // CS5
        if(resp & 32) keys |= KEY_ESC;   // CS6
    }
    else
    {
        if(gpio_readPin(ENTER_SW) == 1) keys |= KEY_ENTER;
        if(gpio_readPin(ESC_SW)   == 1) keys |= KEY_ESC;
        if(gpio_readPin(LEFT_SW)  == 1) keys |= KEY_LEFT;
        if(gpio_readPin(RIGHT_SW) == 1) keys |= KEY_RIGHT;
        if(gpio_readPin(UP_SW)    == 1) keys |= KEY_UP;
        if(gpio_readPin(DOWN_SW)  == 1) keys |= KEY_DOWN;
    }

    return keys;
}
