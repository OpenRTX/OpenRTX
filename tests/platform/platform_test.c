/***************************************************************************
 *   Copyright (C) 2020 - 2025 by Federico Izzo IU2NUO,                    *
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
#include "core/graphics.h"
#include "hwconfig.h"
#include "interfaces/platform.h"
#include "interfaces/delays.h"
#include "interfaces/display.h"

int main(void)
{
    platform_init();
    gfx_init();

    display_setBacklightLevel(255);

    while(1)
    {
        gfx_clearScreen();

        point_t pos_line = {5, 15};
        color_t color_white = {255, 255, 255, 255};

        uint16_t vBat = platform_getVbat();
        uint8_t micLevel = platform_getMicLevel();
        uint8_t volumeLevel = platform_getVolumeLevel();
        int8_t currentCh = platform_getChSelector();
        bool ptt = platform_getPttStatus();
        bool pwr = platform_pwrButtonStatus();

        gfx_print(pos_line, FONT_SIZE_6PT, TEXT_ALIGN_LEFT,
                  color_white, "bat: %d, mic: %d", vBat, micLevel);

        pos_line.y = 30;
        gfx_print(pos_line, FONT_SIZE_6PT, TEXT_ALIGN_LEFT,
                  color_white, "vol: %d, ch: %d", volumeLevel, currentCh);

        pos_line.y = 45;
        gfx_print(pos_line, FONT_SIZE_6PT, TEXT_ALIGN_LEFT,
                  color_white, "ptt: %s, pwr: %s", ptt ? "on" : "off",
                  pwr ? "on" : "off");

        gfx_render();
        sleepFor(0, 250u);
    }

    return 0;
}
