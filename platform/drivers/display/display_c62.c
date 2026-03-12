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

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(display, LOG_LEVEL_DBG);

// ST7735R is a color display using RGB565 format (16 bits per pixel)
#define FB_SIZE (CONFIG_SCREEN_HEIGHT * CONFIG_SCREEN_WIDTH * 2)

static const struct device *displayDev;
static struct display_buffer_descriptor displayBufDesc = {
    .buf_size = FB_SIZE,
    .width = CONFIG_SCREEN_WIDTH,
    .height = CONFIG_SCREEN_HEIGHT,
    .pitch = CONFIG_SCREEN_WIDTH,
};

static uint8_t shadowBuffer[FB_SIZE];

void display_init()
{
    LOG_INF("Display init");
    displayDev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

    if (!device_is_ready(displayDev)) {
        LOG_ERR("Display device not ready");
        return;
    }

    // Turn display on
    display_blanking_off(displayDev);
    LOG_INF("Display init done");
}

void display_terminate()
{
    if (displayDev != NULL) {
        display_blanking_on(displayDev);
    }
}

void display_renderRows(uint8_t startRow, uint8_t endRow, void *fb)
{
    if (startRow >= CONFIG_SCREEN_HEIGHT || endRow >= CONFIG_SCREEN_HEIGHT || startRow > endRow) {
        return;
    }

    uint8_t *frameBuffer = (uint8_t *) fb;
    uint16_t rowSize = CONFIG_SCREEN_WIDTH * 2; // 2 bytes per pixel
    uint16_t rowCount = endRow - startRow + 1;

    // Set up buffer descriptor for partial update
    struct display_buffer_descriptor bufDesc = {
        .buf_size = rowSize * rowCount,
        .width = CONFIG_SCREEN_WIDTH,
        .height = rowCount,
        .pitch = CONFIG_SCREEN_WIDTH,
    };

    // Calculate start position in buffer
    size_t offset = startRow * rowSize;
    uint8_t *srcPtr = frameBuffer + offset;

    // Copy data to shadow buffer with format conversion if needed
    // This example assumes input buffer is already in RGB565 format
    memcpy(shadowBuffer, srcPtr, rowSize * rowCount);

    // Write to display
    display_write(displayDev, 0, startRow, &bufDesc, shadowBuffer);
}


void display_render(void *fb)
{
    uint8_t *frameBuffer = (uint8_t *) fb;

    // Process each 16-bit pixel
    for (size_t i = 0; i < FB_SIZE; i += 2) {
        // Extract the 16-bit RGB565 value
        uint16_t pixel = (frameBuffer[i] << 8) | frameBuffer[i+1];

        // Store in shadow buffer with byte order swapped
        // This fixes endianness issues which can cause color problems
        shadowBuffer[i+1] = (pixel >> 8) & 0xFF;
        shadowBuffer[i] = pixel & 0xFF;
    }

    display_write(displayDev, 0, 0, &displayBufDesc, shadowBuffer);
}

void display_setContrast(uint8_t contrast)
{
    // ST7735R doesn't have a direct contrast control
    // Could implement via gamma correction if needed
    (void) contrast;
}

void display_setBacklightLevel(uint8_t level)
{
    platform_set_display_brightness(level);
}
