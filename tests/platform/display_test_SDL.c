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

/**
 * Testing module for SDL-based display driver, serves both to check that
 * everything is fine and as a simple example on how to use both the driver and
 * the SDL platform.
 *
 * To adjust screen dimensions you have to adjust the corresponding constants in
 * the driver source file.
 */

#include "lcd.h"
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#undef main     //necessary to avoid conflicts with SDL_main


void drawRect(int x, int y, int width, int height, uint16_t color)
{
    int x_max = x + width;
    int y_max = y + height;
    uint16_t *buf = lcd_getFrameBuffer();

    for(int i=y; i < y_max; i++)
    {
        for(int j=x; j < x_max; j++)
        {
            buf[j + i*lcd_screenWidth()] = color;
        }
    }
}

int main()
{
    lcd_init();
    lcd_setBacklightLevel(254);

    /* Horizontal red line */
    drawRect(0, 10, lcd_screenWidth(), 20, 0xF800);

    /* Vertical blue line */
    drawRect(10, 0, 20, lcd_screenHeight(), 0x001F);

    /* Vertical green line */
    drawRect(80, 0, 20, lcd_screenHeight(), 0x07e0);

    /*
     * Use SDL event listener to check if window close button has been pressed,
     * in this case quit.
     */
    SDL_Event eventListener;

    while(1)
    {
        lcd_render();
        SDL_PollEvent(&eventListener);
        if(eventListener.type == SDL_QUIT) break;
    }
    
    lcd_terminate();
    return 0;
}
