/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <zephyr/drivers/display.h>
#include "interfaces/display.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include "hwconfig.h"
#include <string.h>

// Display is monochromatic, one bit per pixel
#define FB_SIZE ((CONFIG_SCREEN_HEIGHT * CONFIG_SCREEN_WIDTH) / 8 + 1)

static const struct device *displayDev;
static const struct display_buffer_descriptor displayBufDesc =
{
    FB_SIZE,
    CONFIG_SCREEN_WIDTH,
    CONFIG_SCREEN_HEIGHT,
    CONFIG_SCREEN_WIDTH,
};

static uint8_t shadowBuffer[FB_SIZE];

void display_init()
{
    // Get display handle
    displayDev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
}

void display_terminate()
{
}

void display_renderRows(uint8_t startRow, uint8_t endRow, void *fb)
{
    (void) startRow;
    (void) endRow;

    // Only complete rendering is supported
    display_render(fb);
}

void display_render(void *fb)
{
    uint8_t *frameBuffer = (uint8_t *) fb;
    memset(shadowBuffer, 0x00, FB_SIZE);

    for(uint8_t y = 0; y < CONFIG_SCREEN_HEIGHT; y++)
    {
        for(uint8_t x = 0; x < CONFIG_SCREEN_WIDTH; x++)
        {
            size_t cell = x / 8 + y * (CONFIG_SCREEN_WIDTH / 8);
            bool pixel = frameBuffer[cell] & (1 << (x % 8));
            if (pixel)
                shadowBuffer[x + y / 8 * CONFIG_SCREEN_WIDTH] |= 1 << (y % 8);
        }
    }

    display_write(displayDev, 0, 0, &displayBufDesc, shadowBuffer);
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
