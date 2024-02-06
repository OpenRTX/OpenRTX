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
#include <interfaces/gpio.h>
#include <interfaces/graphics.h>
#include "hwconfig.h"
#include <interfaces/platform.h>

void platform_test()
{
    gpio_togglePin(GREEN_LED);
    OS_ERR os_err;
    Pos_st pos_line1 = {0, 0};
    Pos_st pos_line2 = {0, 9};
    Pos_st pos_line3 = {0, 17};
    Color_st color_fg = {255, 255, 255};
    uiColorLoad( &color_fg , COLOR_FG );
    gfx_print(pos_line1, FONT_SIZE_1, ALIGN_LEFT,
              color_fg, "Platform Test");
    float vBat = platform_getVbat();
    float micLevel = platform_getMicLevel();
    float volumeLevel = platform_getVolumeLevel();
    uint8_t currentCh = platform_getChSelector();
    bool ptt = platform_getPttStatus();
    gfx_print(pos_line2, FONT_SIZE_1, ALIGN_LEFT,
              color_fg, "bat:%.2f mic:%.2f", vBat, micLevel);
    gfx_print(pos_line3, FONT_SIZE_1, ALIGN_LEFT,
              color_fg, "vol:%.2f ch:%d ptt:%s", volumeLevel,
              currentCh, ptt?"on":"off");
    gfx_render();
    while(gfx_renderingInProgress());
    OSTimeDlyHMSM(0u, 0u, 0u, 250u, OS_OPT_TIME_HMSM_STRICT, &os_err);
}

int main(void)
{
    gpio_setMode(GREEN_LED, OUTPUT);

    // Init the graphic stack
    gfx_init();
    platform_setBacklightLevel(255);

    Pos_st origin = {0, SCREEN_HEIGHT / 2};
    Color_st color_op3 ;
    uiColorLoad( &color_op3 , COLOR_OP3 );

    OS_ERR os_err;

    // Task infinite loop
    while(1)
    {
        gfx_clearScreen();
        gfx_print(origin, FONT_SIZE_4, ALIGN_CENTER,
                  color_op3, "OpenRTX");
        //gfx_render();
        //while(gfx_renderingInProgress());
        platform_test();
        OSTimeDlyHMSM(0u, 0u, 0u, 100u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}
