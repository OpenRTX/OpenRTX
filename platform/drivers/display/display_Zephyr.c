/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                Mathis Schmieder DB9MAT                  *
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

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <interfaces/display.h>
#include <hwconfig.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

/*
 * LCD framebuffer, statically allocated and placed in the "large" RAM block
 * starting at 0x20000000.
 * Pixel format is black and white, one bit per pixel.
 */
#define FB_SIZE ((SCREEN_HEIGHT * SCREEN_WIDTH) / 8 + 1)
static uint8_t frameBuffer[FB_SIZE];

const struct device *displayDev;
const struct display_buffer_descriptor displayBufDesc = {
    FB_SIZE,
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    SCREEN_WIDTH,
};

void display_init()
{
    // Get display handle and framebuffer pointer
    displayDev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    // Clear framebuffer, setting all pixels to 0x00 makes the screen black
    memset(&frameBuffer, 0x00, FB_SIZE);
    display_write(displayDev, 0, 0, &displayBufDesc, frameBuffer);
}

void display_terminate()
{
    // Nothing to do here
    ;
}

void display_renderRows(uint8_t startRow, uint8_t endRow)
{
    // Only complete rendering is supported
    display_render();
}

void display_render()
{
#if defined(CONFIG_SSD1306)

#define ROW_HEIGHT 8
    /*
     * \internal
     * Send one row of pixels to the display.
     * Pixels in framebuffer are stored "by rows", while display needs data to be
     * sent "by columns": this function performs the needed conversion.
     */
    static uint8_t shadowBuffer[FB_SIZE] = { 0 };
    for(uint8_t y = 0; y < SCREEN_HEIGHT; y++)
    {
        for(uint8_t x = 0; x < SCREEN_WIDTH; x++)
        {
            size_t cell = x / 8 + y * (SCREEN_WIDTH / ROW_HEIGHT);
            bool pixel = frameBuffer[cell] & (1 << (x % 8));
            if (pixel)
                shadowBuffer[x + y / 8 * SCREEN_WIDTH] |= 1 << (y % 8);
        }
    }
#endif
    display_write(displayDev, 0, 0, &displayBufDesc, shadowBuffer);
}

bool display_renderingInProgress()
{
    return false;
}

void *display_getFrameBuffer()
{
    return (void *)(frameBuffer);
}

void display_setContrast(uint8_t contrast)
{
    display_set_contrast(displayDev, contrast);
}
