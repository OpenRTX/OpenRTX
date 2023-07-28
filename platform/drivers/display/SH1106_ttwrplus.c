/***************************************************************************
 *   Copyright (C) 2023 by Niccolò Izzo IU2KIN                             *
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

#include <zephyr/drivers/display.h>
#include <interfaces/display.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <hwconfig.h>
#include <string.h>

// Display is monochromatic, one bit per pixel
#define FB_SIZE ((SCREEN_HEIGHT * SCREEN_WIDTH) / 8 + 1)

static const struct device *displayDev;
static const struct display_buffer_descriptor displayBufDesc =
{
    FB_SIZE,
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    SCREEN_WIDTH,
};

static uint8_t frameBuffer[FB_SIZE];
static bool    rendering = false;

void display_init()
{
    // Get display handle
    displayDev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

    // Clear framebuffer, setting all pixels to 0x00 makes the screen black
    memset(&frameBuffer, 0x00, FB_SIZE);
    display_write(displayDev, 0, 0, &displayBufDesc, frameBuffer);
}

void display_terminate()
{
}

void display_renderRows(uint8_t startRow, uint8_t endRow)
{
    // Only complete rendering is supported
    display_render();
}

void display_render()
{
    static uint8_t shadowBuffer[FB_SIZE] = { 0 };
    memset(shadowBuffer, 0x00, FB_SIZE);

    rendering = true;
    for(uint8_t y = 0; y < SCREEN_HEIGHT; y++)
    {
        for(uint8_t x = 0; x < SCREEN_WIDTH; x++)
        {
            size_t cell = x / 8 + y * (SCREEN_WIDTH / 8);
            bool pixel = frameBuffer[cell] & (1 << (x % 8));
            if (pixel)
                shadowBuffer[x + y / 8 * SCREEN_WIDTH] |= 1 << (y % 8);
        }
    }

    display_write(displayDev, 0, 0, &displayBufDesc, shadowBuffer);
    rendering = false;
}

bool display_renderingInProgress()
{
    return rendering;
}

void *display_getFrameBuffer()
{
    return (void *)(frameBuffer);
}

void display_setContrast(uint8_t contrast)
{
    display_set_contrast(displayDev, contrast);
}

void display_setBacklightLevel(uint8_t level)
{
    if(level > 100)
        level = 100;

    uint8_t bkLevel = (2 * level) + (level * 55)/100;    // Convert value to 0 - 255
    display_set_contrast(displayDev, bkLevel);
}
