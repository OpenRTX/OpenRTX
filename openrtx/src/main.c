/***************************************************************************
 *   Copyright (C) 2020 by Federico Izzo IU2NUO, Niccolò Izzo IU2KIN and   *
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


int main(void)
{
    OS_ERR os_err;

    // Initialize the radio state
    state_init();
    
    // Init the graphic stack
    gfx_init();
    platform_setBacklightLevel(255);

    // Print splash screen
    point_t splash_origin = {0, SCREEN_HEIGHT / 2};
    color_t color_yellow_fab413 = {250, 180, 19};
    char *splash_buf = "OpenRTX";
    gfx_clearScreen();
    gfx_print(splash_origin, splash_buf, FONT_SIZE_4, TEXT_ALIGN_CENTER, color_yellow_fab413);
    gfx_render();
    while(gfx_renderingInProgress());
    OSTimeDlyHMSM(0u, 0u, 1u, 0u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    
    // Clear screen
    gfx_clearScreen();
    gfx_render();
    while(gfx_renderingInProgress());

    // UI update infinite loop
    while(1)
    {
	state_t state = state_update();
	uint32_t keys = kbd_getKeys();
	bool renderNeeded = ui_update(state, keys);
	if(renderNeeded)
	{
	    gfx_render();
	    while(gfx_renderingInProgress());
	}
        OSTimeDlyHMSM(0u, 0u, 0u, 100u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}
