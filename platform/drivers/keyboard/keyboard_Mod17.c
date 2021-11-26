/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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
#include <interfaces/gpio.h>
#include <interfaces/delays.h>
#include <interfaces/keyboard.h>
#include <interfaces/platform.h>
#include <hwconfig.h>

void kbd_init()
{
    gpio_setMode(ESC_SW,   INPUT);
    gpio_setMode(ENTER_SW, INPUT);
    gpio_setMode(LEFT_SW,  INPUT);
    gpio_setMode(RIGHT_SW, INPUT);
    gpio_setMode(UP_SW,    INPUT);
    gpio_setMode(DOWN_SW,  INPUT);
}

void kbd_terminate()
{

}

keyboard_t kbd_getKeys()
{
    keyboard_t keys = 0;

    if(gpio_readPin(ENTER_SW) == 1) keys |= KEY_ENTER;
    if(gpio_readPin(ESC_SW)   == 1) keys |= KEY_ESC;
    if(gpio_readPin(LEFT_SW)  == 1) keys |= KEY_LEFT;
    if(gpio_readPin(RIGHT_SW) == 1) keys |= KEY_RIGHT;
    if(gpio_readPin(UP_SW)    == 1) keys |= KEY_UP;
    if(gpio_readPin(DOWN_SW)  == 1) keys |= KEY_DOWN;

    return keys;
}
