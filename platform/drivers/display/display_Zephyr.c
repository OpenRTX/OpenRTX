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

const struct device *display_dev;
size_t fb_size = 0;
static uint8_t *frameBuffer;

void display_init()
{
    // Get display handle and framebuffer pointer
    display_dev = DEVICE_DT_GET(DT_ALIAS(display));
    frameBuffer = display_get_framebuffer(display_dev);
    struct display_capabilities display_cap;
    display_get_capabilities(display_dev, &display_cap);
    fb_size = DISPLAY_BITS_PER_PIXEL(display_cap.current_pixel_format) *
              display_cap.x_resolution * display_cap.y_resolution / 8;
    // Blank display to make framebuffer accessible
    display_blanking_on(display_dev);
    // Clear framebuffer, setting all pixels to 0x00 makes the screen white
    memset(frameBuffer, 0x00, fb_size);
    // Display framebuffer
    display_blanking_off(display_dev);
}

void display_terminate()
{
    // Clear display content
    display_blanking_on(display_dev);
    // Clear framebuffer, setting all pixels to 0x00 makes the screen white
    memset(frameBuffer, 0x00, fb_size);
    // Display framebuffer
    display_blanking_off(display_dev);
}

void display_unlockRows(uint8_t startRow, uint8_t endRow)
{
    display_blanking_on(display_dev);
}

void display_unlock()
{
    display_unlockRows(0, SCREEN_HEIGHT);
}

void display_renderRows(uint8_t startRow, uint8_t endRow)
{
    display_blanking_off(display_dev);
}

void display_render()
{
    display_renderRows(0, SCREEN_HEIGHT);
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
    display_set_contrast(display_dev, contrast);
}
