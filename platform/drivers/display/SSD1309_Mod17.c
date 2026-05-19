/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "peripherals/gpio.h"
#include "interfaces/delays.h"
#include "hwconfig.h"
#include "drivers/SPI/spi_stm32.h"
#include "SSD1309_Mod17.h"

extern const struct spiDevice spi2;

void SSD1309_init()
{
    gpio_setPin(LCD_CS);
    gpio_clearPin(LCD_DC);

    gpio_clearPin(LCD_RST); // Reset controller
    delayMs(50);
    gpio_setPin(LCD_RST);
    delayMs(50);

    static const uint8_t init[] =
    {
        0xAE,    // SSD1309_DISPLAYOFF,
        0xD5,    // Set display clock division
        0xF0,
        0xA8,    // Set multiplex ratio, 1/64
        0x3F,
        0x81,    // Set contrast control
        0x32,
        0xD9,    // Set pre-charge period
        0xF1,
        0xDB,    // Set VCOMH Deselect level
        0x30,
        0xAF
    };

    gpio_clearPin(LCD_CS);
    gpio_clearPin(LCD_DC);
    spi_send(&spi2, init, sizeof(init));
    gpio_setPin(LCD_CS);
}

void SSD1309_terminate()
{
    uint8_t dispOff = 0xAE;

    gpio_clearPin(LCD_CS);
    gpio_clearPin(LCD_DC);  /* DC low -> command mode          */
    spi_send(&spi2, &dispOff, 1);
    gpio_setPin(LCD_CS);
}

void SSD1309_renderRows(uint8_t startRow, uint8_t endRow, void *fb)
{
    gpio_clearPin(LCD_CS);

    // Convert rows to pages
    uint8_t startPage = startRow / 8;
    uint8_t endPage = endRow / 8;
    uint8_t cmd[3];

    gpio_clearPin(LCD_DC);
    cmd[0] = 0x20; // Set page addressing mode
    cmd[1] = 0x02;
    spi_send(&spi2, cmd, 2);

    uint8_t *framebuffer = (uint8_t *)fb;

    for(uint8_t page = startPage; page < endPage; page++)
    {
        gpio_clearPin(LCD_DC);
        cmd[0] = 0xB0 | page;
        cmd[1] = 0x00;
        cmd[1] = 0x10;
        spi_send(&spi2, cmd, 3);
        gpio_setPin(LCD_DC); // DC high -> data mode

        uint8_t topRow = page * 8;
        for(uint8_t col = 0; col < CONFIG_SCREEN_WIDTH; col++)
        {
            uint8_t data = 0;
            uint8_t bit_offset = col % 8; // Bit offset in the fb for the column we are refreshing
            // Gather the 8 rows of data
            for(uint8_t row = 0; row < 8; row++)
            {
                size_t pos = ((topRow + row) * CONFIG_SCREEN_WIDTH + col) / 8;
                uint8_t cell = framebuffer[pos];
                data |= ((cell >> bit_offset) & 0x01) << row;
            }

            spi_send(&spi2, &data, 1);
        }
    }

    gpio_setPin(LCD_CS);
}

void SSD1309_render(void *fb)
{
    static const uint8_t cmd[] =
    {
        0xB0,
        0x20, // Set horizontal addressing mode
        0x00,
        0x00,
        0x10
    };

    gpio_clearPin(LCD_CS);
    gpio_clearPin(LCD_DC);
    spi_send(&spi2, cmd, sizeof(cmd));
    gpio_setPin(LCD_DC); // DC high -> data mode

    uint8_t *framebuffer = (uint8_t *)fb;

    // Refresh the whole screen 8 rows by 8 rows
    for(uint8_t topRow = 0; topRow <= 56; topRow += 8)
    {
        for(uint8_t col = 0; col < CONFIG_SCREEN_WIDTH; col++)
        {
            uint8_t data = 0;
            uint8_t bit_offset = col % 8; // Bit offset in the fb for the column we are refreshing
            // Gather the 8 rows of data
            for(uint8_t subRow = 0; subRow < 8; subRow++)
            {
                size_t pos = ((topRow + subRow) * CONFIG_SCREEN_WIDTH + col) / 8;
                uint8_t cell = framebuffer[pos];
                data |= ((cell >> bit_offset) & 0x01) << subRow;
            }

            spi_send(&spi2, &data, 1);
        }
    }

    gpio_setPin(LCD_CS);
}

void SSD1309_setContrast(uint8_t contrast)
{
    uint8_t cmd[2];
    cmd[0] = 0x81;          /* Set Electronic Volume               */
    cmd[0] = contrast;      /* Controller contrast range is 0 - 63 */

    gpio_clearPin(LCD_CS);
    gpio_clearPin(LCD_DC);  /* RS low -> command mode              */
    spi_send(&spi2, cmd, sizeof(cmd));
    gpio_setPin(LCD_CS);
}
