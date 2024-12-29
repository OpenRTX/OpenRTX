/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Izzo IU2NUO,                    *
 *                                Niccolò Izzo IU2KIN                      *
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <os.h>
#include <peripherals/gpio.h>
#include <interfaces/platform.h>
#include <interfaces/display.h>
#include <interfaces/delays.h>
#include <graphics.h>
#include <hwconfig.h>
#include <pinmap.h>

bool toggle = true;

void platform_test()
{
    if (toggle) {
        gpio_clearPin(GREEN_LED);
    } else {
        gpio_setPin(GREEN_LED);
    }
    toggle = !toggle;
    point_t pos_line1 = {0, 0};
    point_t pos_line2 = {0, 9};
    point_t pos_line3 = {0, 17};
    color_t color_white = {255, 255, 255, 255};
    gfx_print(pos_line1, FONT_SIZE_5PT, TEXT_ALIGN_LEFT,
              color_white, "Platform Test");
    float vBat = platform_getVbat();
    float micLevel = platform_getMicLevel();
    float volumeLevel = platform_getVolumeLevel();
    uint8_t currentCh = platform_getChSelector();
    bool ptt = platform_getPttStatus();
    gfx_print(pos_line2, FONT_SIZE_5PT, TEXT_ALIGN_LEFT,
              color_white, "bat:%.2f mic:%.2f", vBat, micLevel);
    gfx_print(pos_line3, FONT_SIZE_5PT, TEXT_ALIGN_LEFT,
              color_white, "vol:%.2f ch:%d ptt:%s", volumeLevel, 
              currentCh, ptt?"on":"off");
    gfx_render();
    delayMs(250);
}

int main(void)
{
    gpio_setMode(GREEN_LED, OUTPUT);

    // Init the graphic stack
    gfx_init();
    display_init();
    display_setBacklightLevel(100);

    point_t origin = {0, CONFIG_SCREEN_HEIGHT / 2};
    color_t color_yellow = {250, 180, 19, 255};


    // Task infinite loop
    while(1)
    {
        gfx_clearScreen();
        gfx_print(origin, FONT_SIZE_8PT, TEXT_ALIGN_CENTER,
                  color_yellow, "OpenRTX");
        //gfx_render();
        //while(gfx_renderingInProgress());
        platform_test();
        delayMs(100);
    }
}
