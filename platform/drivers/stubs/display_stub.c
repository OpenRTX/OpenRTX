/***************************************************************************
 *   Copyright (C) 2023 by Federico Izzo IU2NUO,                           *
 *                         Niccolò Izzo IU2KIN,                            *
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

#include <interfaces/display.h>
#include <hwconfig.h>
#include <stddef.h>

#ifdef CONFIG_PIX_FMT_BW
static uint8_t frameBuffer[(((CONFIG_SCREEN_WIDTH * CONFIG_SCREEN_HEIGHT) / 8 ) + 1)];
#else
static uint16_t frameBuffer[CONFIG_SCREEN_WIDTH * CONFIG_SCREEN_HEIGHT];
#endif


void display_init()
{

}

void display_terminate()
{

}

void display_renderRows(uint8_t startRow, uint8_t endRow)
{
    (void) startRow;
    (void) endRow;
}

void display_render()
{

}

bool display_renderingInProgress()
{
    return false;
}

void *display_getFrameBuffer()
{
    return (void *) (frameBuffer);
}

void display_setContrast(uint8_t contrast)
{
    (void) contrast;
}

void display_setBacklightLevel(uint8_t level)
{
    (void) level;
}
