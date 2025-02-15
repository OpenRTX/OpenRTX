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
#include <graphics.h>
#include "hwconfig.h"
#include <interfaces/platform.h>
#include "state.h"
#include <interfaces/keyboard.h>
#include <interfaces/delays.h>
#include <interfaces/display.h>
#include "ui.h"


int main(void)
{

    // Initialize the radio state
    state_init();
    
    // Init the graphic stack
    gfx_init();
    display_setBacklightLevel(100);

    // Print splash screen
    point_t splash_origin = {0, CONFIG_SCREEN_HEIGHT / 2};
    color_t color_yellow_fab413 = {250, 180, 19, 255};
    gfx_clearScreen();
    gfx_print(splash_origin, FONT_SIZE_8PT, TEXT_ALIGN_CENTER,
              color_yellow_fab413, "OpenRTX");
    gfx_render();
    delayMs(1000);
    
    // Clear screen
    gfx_clearScreen();
    gfx_render();

    // UI update infinite loop
    while(1)
    {
	bool renderNeeded = ui_updateGUI();
	if(renderNeeded)
	{
	    gfx_render();
	}
        delayMs(100);
    }
}
