/***************************************************************************
 *   Copyright (C) 2020 - 2025 by Federico Amedeo Izzo IU2NUO,             *
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "hwconfig.h"
#include "core/graphics.h"
#include "interfaces/platform.h"
#include "interfaces/keyboard.h"
#include "interfaces/display.h"
#include "interfaces/delays.h"

char *keys_list[] = {
     " ", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
     "*", "#", "ENTER", "ESC", "UP", "DOWN", "LEFT", "RIGHT",
     "MONI", "F1"
};

int main(void)
{
    platform_init();
    gfx_init();
    kbd_init();

    gfx_clearScreen();
    gfx_render();
    display_setBacklightLevel(255);

    while (1) {

        gfx_clearScreen();

        color_t color_white = {255, 255, 255, 255};
        color_t color_green = {0,   255,   0, 255};
        point_t origin = {5, 10};

        gfx_print(origin, FONT_SIZE_6PT, TEXT_ALIGN_LEFT, color_green,
                    "Keyboard test");

        keyboard_t keys = kbd_getKeys();
        int i = __builtin_popcount(keys);

        while (i > 0) {
            //position of the first set bit
            int pos = __builtin_ctz(keys);

            origin.y += 10;
            gfx_print(origin, FONT_SIZE_6PT, TEXT_ALIGN_LEFT, color_white,
                        "Pressed: %s", keys_list[pos + 1]);

            //unset the bit we already handled
            keys &= ~(1 << pos);
            i--;
        }

        gfx_render();
        sleepFor(0, 100u);
    }

    return 0;
}

