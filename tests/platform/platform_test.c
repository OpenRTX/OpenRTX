/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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
