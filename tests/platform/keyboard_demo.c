/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <os.h>
#include "gpio.h"
#include "graphics.h"
#include "hwconfig.h"
#include "platform.h"
#include "state.h"
#include "keyboard.h"
#include "ui.h"

color_t color_yellow_fab413 = {250, 180, 19};
color_t color_red = {255, 0, 0};
color_t color_green = {0, 255, 0};

char *keys_list[] = {
        " ", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "*", "#", "ENTER", "ESC", "UP", "DOWN", "LEFT", "RIGHT",
        "MONI", "F1"
};

void *print_keys(keyboard_t keys) {
    point_t origin = {0, SCREEN_HEIGHT / 4};
    //count set bits to check how many keys are being pressed
    int i = __builtin_popcount(keys);
    while (i > 0) {
        char *buf[15];
        //position of the first set bit
        int pos = __builtin_ctz(keys);
        sprintf(buf, "Pressed: %s", keys_list[pos + 1]);
        gfx_print(origin, buf, FONT_SIZE_8PT, TEXT_ALIGN_LEFT, color_green);
        origin.y += 9;
        //unset the bit we already handled
        keys &= ~(1 << pos);
        i--;
    }
    gfx_render();
    while (gfx_renderingInProgress());
}

int main(void) {
    OS_ERR os_err;

    // Initialize platform drivers
    platform_init();

    // Initialize graphics driver
    gfx_init();

    // Clear screen
    gfx_clearScreen();
    gfx_render();
    while (gfx_renderingInProgress());
    platform_setBacklightLevel(255);

    // Initialize keyboard driver
    kbd_init();

    point_t title_origin = {0, SCREEN_HEIGHT / 9};

    char *title_buf = "Keyboard demo";

    // UI update infinite loop
    while (1) {
        gfx_clearScreen();
        gfx_print(title_origin, title_buf, FONT_SIZE_8PT, TEXT_ALIGN_CENTER, color_red);
        keyboard_t keys = kbd_getKeys();
        if (keys != 0)
            print_keys(keys);
        OSTimeDlyHMSM(0u, 0u, 0u, 100u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}

