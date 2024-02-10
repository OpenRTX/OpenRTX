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
#include <I2C2.h>
#include "cap1206.h"

static const hwInfo_t *hwinfo = NULL;
static Mod17_HwInfo_t *mod17_hw_info = NULL;
void kbd_init()
{
    hwinfo = platform_getHwInfo();
    mod17_hw_info = (Mod17_HwInfo_t*)hwinfo->other;

    if((mod17_hw_info->HMI_present) 
        && (mod17_hw_info->HMI_hw_version == CONFIG_HMI_HWVER_1_0))
    {
        // Init CAP1206 tactile switch controller
        cap1206_init();
    }

}

void kbd_terminate()
{

}

keyboard_t kbd_getKeys()
{
    keyboard_t keys = 0;

    if( ((Mod17_HwInfo_t*)hwinfo->other)->HMI_present 
        && ((Mod17_HwInfo_t*)hwinfo->other)->HMI_hw_version == CONFIG_HMI_HWVER_1_0)
    {
        uint8_t resp = cap1206_readkeys();
        if(resp & 1) keys |= KEY_LEFT;  // CS1
        if(resp & 2) keys |= KEY_DOWN;  // CS2
        if(resp & 4) keys |= KEY_RIGHT; // CS3
        if(resp & 8) keys |= KEY_ENTER; // CS4
        if(resp & 16) keys |= KEY_UP;   // CS5
        if(resp & 32) keys |= KEY_ESC;  // CS6
    }
    else if( ((Mod17_HwInfo_t*)hwinfo->other)->HMI_hw_version == false )
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
