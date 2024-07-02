/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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
#include <os.h>
#include <interfaces/gpio.h>
#include <interfaces/graphics.h>
#include "hwconfig.h"
#include <interfaces/platform.h>
#include "state.h"
#include <interfaces/keyboard.h>
#include "ui.h"

char *keys_list[] = {
        " ", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "*", "#", "ENTER", "ESC", "UP", "DOWN", "LEFT", "RIGHT",
        "MONI", "F1"
};

void *print_keys(keyboard_t keys)
{
    Color_st color_op3 ;
    ui_ColorLoad( &color_op3 , COLOR_OP3 );
    Color_st color_op0 ;
    ui_ColorLoad( &color_op0 , COLOR_OP0 );
    Color_st color_op1 ;
    ui_ColorLoad( &color_op1 , COLOR_OP1 );

    Pos_st origin = {0, SCREEN_HEIGHT / 4};
    //count set bits to check how many keys are being pressed
    int i = __builtin_popcount(keys);
    while (i > 0) {
        //position of the first set bit
        int pos = __builtin_ctz(keys);
        gfx_print(origin, FONT_SIZE_8PT, GFX_ALIGN_LEFT,
                  color_op1, "Pressed: %s", keys_list[pos + 1]);
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

    Pos_st title_origin = {0, SCREEN_HEIGHT / 9};

    // UI update infinite loop
    while (1) {
        gfx_clearScreen();
        gfx_print(title_origin, FONT_SIZE_8PT, GFX_ALIGN_CENTER,
                  color_op0, "Keyboard demo");
        keyboard_t keys = kbd_getKeys();
        if (keys != 0)
            print_keys(keys);
        OSTimeDlyHMSM(0u, 0u, 0u, 100u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}

